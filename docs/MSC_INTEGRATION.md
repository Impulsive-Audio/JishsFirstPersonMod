# MSC first-person nametags

On the supported Midnight Skate Club dedicated server, `[FP]` is added to your
multiplayer display name while the mod's First Person setting is enabled.
Temporary third-person camera changes do not clear the indicator. There is no
additional toggle, and Steam names are never changed.

Reporting starts only after an authenticated dedicated-server session has both
a current roster name beginning exactly with `-MSC-Midnight Skate Club-` and
matching first-person capability metadata for that server and session. The
server-name suffix can change. The supported server must be publicly listed for
Steam capability discovery. Unsupported or unavailable discovery leaves this
optional integration inactive without affecting normal mod operation.

Presence updates use the existing server-command channel. Enabled status is
refreshed every five seconds; the matching server expires it after fifteen
seconds without a heartbeat. Disconnecting, changing sessions or losing the
required server-name prefix clears client confirmation. A fresh session must
qualify again before any presence commands are sent.

There are no feature chat posts, acknowledgements or error messages. The server
changes outgoing roster name copies, preserving its original player names,
identities and moderation behavior. Because stock UI shares these roster names,
the prefix can also appear in player lists or beside a normal chat sender's
name; the integration itself generates no chat messages.

The client bridge supports the exact official ReSkate 2.0.7 binary and checks
its native entry points before enabling. Other builds leave the integration
inactive. No protocol versions, packet layouts or Steam authentication are
changed, and ordinary clients need no mod or presence response to join.

This release includes only the client integration and its matching source.
Custom server binaries and server source are distributed separately.

## Validation

The user confirmed the combined MSC client/server integration works in-game.
Automated tests cover server/session gating, discovery metadata, setting
changes, heartbeats and stale queued commands. This does not imply every live
Steam authentication or multiplayer configuration has been independently tested.
