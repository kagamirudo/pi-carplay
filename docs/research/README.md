# Research notes (academic)

Background material for a paper on CarPlay/MFi interoperability. This folder is
documentation only: how the protocols work from public sources, prior
reverse-engineering art, and interop notes for talking to a **licensed** adapter
(e.g. Carlinkit) from this head unit.

## Scope

In scope:
- How MFi authentication works, from Apple's public docs and published research.
- Prior art: OpenAuto/aasdk, react-carplay, WirelessAndroidAutoDongle, dongle teardowns.
- Interop with a licensed adapter: USB video/audio/input framing that does not touch the auth path.

Out of scope (do not add here):
- Extracted or emulated MFi keys, or any circumvention of the authentication coprocessor.
- Copied proprietary firmware.

## Before publishing

Circumvention research can fall under DMCA §1201; the research exemption is narrow.
Confirm the university's IRB/legal position before publishing anything in this folder.
