#!/usr/bin/env python3
"""Small production-oriented licensing API prototype.
Run behind HTTPS (Caddy/nginx/Cloudflare) and never expose the SQLite DB.
"""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json, os, secrets, sqlite3, hashlib, hmac, time
from urllib.parse import urlparse

HOST = os.getenv('LICENSE_HOST', '127.0.0.1')
PORT = int(os.getenv('LICENSE_PORT', '8787'))
DB = os.getenv('LICENSE_DB', 'licenses.db')
ADMIN_TOKEN = os.environ['LICENSE_ADMIN_TOKEN']
SERVER_SECRET = os.environ['LICENSE_SERVER_SECRET'].encode()

PLANS = {'1d': 86400, '1w': 7*86400, '1m': 30*86400, '1y': 365*86400}
ALPHABET = 'ABCDEFGHJKLMNPQRSTUVWXYZ23456789'

def db():
    c = sqlite3.connect(DB)
    c.execute('''CREATE TABLE IF NOT EXISTS licenses (
        key_hash TEXT PRIMARY KEY, plan TEXT NOT NULL, created INTEGER NOT NULL,
        expires INTEGER NOT NULL, device_hash TEXT, revoked INTEGER NOT NULL DEFAULT 0,
        activations INTEGER NOT NULL DEFAULT 0)''')
    c.commit(); return c

def make_key():
    return 'VAL-' + '-'.join(''.join(secrets.choice(ALPHABET) for _ in range(6)) for _ in range(4))

def kh(k): return hashlib.sha256(k.encode()).hexdigest()
def dh(v): return hmac.new(SERVER_SECRET, v.encode(), hashlib.sha256).hexdigest()
def token(khsh, exp, device):
    body=f'{khsh}.{exp}.{device}'
    sig=hmac.new(SERVER_SECRET, body.encode(), hashlib.sha256).hexdigest()
    return body+'.'+sig

def ok_token(t):
    try:
        a,b,c,s=t.split('.')
        body=f'{a}.{b}.{c}'
        if not hmac.compare_digest(s, hmac.new(SERVER_SECRET, body.encode(), hashlib.sha256).hexdigest()): return False
        return int(b) > int(time.time())
    except Exception: return False

class H(BaseHTTPRequestHandler):
    def reply(self, code, obj):
        raw=json.dumps(obj).encode(); self.send_response(code); self.send_header('Content-Type','application/json'); self.send_header('Content-Length',str(len(raw))); self.end_headers(); self.wfile.write(raw)
    def body(self): return json.loads(self.rfile.read(int(self.headers.get('Content-Length','0')) or 0) or b'{}')
    def do_POST(self):
        p=urlparse(self.path).path
        try: data=self.body()
        except Exception: return self.reply(400, {'ok':False,'error':'bad_json'})
        if p == '/admin/generate':
            if not hmac.compare_digest(data.get('admin_token',''), ADMIN_TOKEN): return self.reply(403, {'ok':False,'error':'forbidden'})
            plan=data.get('plan');
            if plan not in PLANS: return self.reply(400, {'ok':False,'error':'plan'})
            k=make_key(); now=int(time.time()); exp=now+PLANS[plan]
            c=db(); c.execute('INSERT INTO licenses VALUES (?,?,?,?,?,?,?)',(kh(k),plan,now,exp,None,0,0)); c.commit(); c.close()
            return self.reply(200, {'ok':True,'key':k,'plan':plan,'expires':exp})
        if p == '/v1/activate':
            k=data.get('key','').strip(); device=data.get('device_id','').strip()
            if len(k) < 20 or not device: return self.reply(400, {'ok':False,'error':'invalid_request'})
            c=db(); row=c.execute('SELECT plan,created,expires,device_hash,revoked,activations FROM licenses WHERE key_hash=?',(kh(k),)).fetchone()
            if not row: c.close(); return self.reply(401, {'ok':False,'error':'invalid_key'})
            plan,created,exp,stored,revoked,acts=row
            now=int(time.time())
            if revoked or exp <= now: c.close(); return self.reply(401, {'ok':False,'error':'expired_or_revoked'})
            d=dh(device)
            if stored and not hmac.compare_digest(stored,d): c.close(); return self.reply(403, {'ok':False,'error':'device_mismatch'})
            c.execute('UPDATE licenses SET device_hash=?,activations=? WHERE key_hash=?',(d,acts+1,kh(k))); c.commit(); c.close()
            return self.reply(200, {'ok':True,'plan':plan,'expires':exp,'token':token(kh(k),exp,d)})
        if p == '/v1/heartbeat':
            t=data.get('token','');
            if not ok_token(t): return self.reply(401, {'ok':False,'error':'invalid_session'})
            a,b,c,s=t.split('.')
            con=db(); row=con.execute('SELECT expires,revoked FROM licenses WHERE key_hash=?',(a,)).fetchone(); con.close()
            if not row or row[1] or row[0] <= int(time.time()): return self.reply(401, {'ok':False,'error':'inactive'})
            return self.reply(200, {'ok':True,'expires':row[0]})
        self.reply(404, {'ok':False,'error':'not_found'})

if __name__ == '__main__':
    db().close(); print(f'Listening on {HOST}:{PORT}')
    ThreadingHTTPServer((HOST,PORT),H).serve_forever()
