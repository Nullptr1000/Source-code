# VAL License Server

This backend issues opaque paid keys and verifies activation/heartbeat server-side.

Plans: `1d`, `1w`, `1m`, `1y`.

## Run

Set `LICENSE_ADMIN_TOKEN` and `LICENSE_SERVER_SECRET` as environment variables, then:

```bash
python3 server/license_server.py
```

Put it behind HTTPS before connecting an Android client. Do not ship either server secret or the admin token in the APK.

## Generate a key

POST `/admin/generate` with JSON containing `admin_token` and `plan`.

## Client flow

1. User enters key in ImGui.
2. Client sends key + a locally generated stable device ID to `/v1/activate` over HTTPS.
3. Server binds the key to the first device and returns a short-lived session token.
4. Client periodically calls `/v1/heartbeat`.
5. If the key is revoked, expired, or moved to another device, the server denies access.

The APK must never contain a master key, admin token, or database.
