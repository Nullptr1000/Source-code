#!/usr/bin/env python3
"""Generate paid VAL keys through the license server.

Usage: LICENSE_ADMIN_TOKEN=... python3 keygen.py 1d
"""
import json, os, sys, urllib.request
if len(sys.argv) != 2 or sys.argv[1] not in {'1d','1w','1m','1y'}:
    raise SystemExit('Usage: keygen.py 1d|1w|1m|1y')
base=os.getenv('LICENSE_SERVER','http://127.0.0.1:8787')
token=os.environ['LICENSE_ADMIN_TOKEN']
data=json.dumps({'admin_token':token,'plan':sys.argv[1]}).encode()
req=urllib.request.Request(base+'/admin/generate',data=data,headers={'Content-Type':'application/json'})
with urllib.request.urlopen(req,timeout=10) as r: print(json.load(r)['key'])
