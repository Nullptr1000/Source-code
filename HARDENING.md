# Paid-key hardening

The old client-only `VAL-<expiry>-<random>` format was intentionally replaced for paid licensing: expiry is no longer encoded in a key that the APK can parse and forge.

The included server stores only SHA-256 key hashes, binds the first activation to a server-derived device hash, supports revocation, and issues a server session token. Keep the server behind HTTPS and keep secrets server-side.

A native-only APK cannot be made impossible to patch. The important security boundary is the server: activation, expiration, revocation, and device binding must be decided there.
