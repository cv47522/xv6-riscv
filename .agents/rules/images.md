# Image rules

Applies to everything under `docs/images/` and every image link in `docs/`.

## Rules

- Every image in a folder that has a `README.md` **must** be listed in that README with its provenance: source, capture method, date, and what it shows. *(lint: `image-provenance`)*
- Name captures `<arch>-<NN>-<topic>.png` and keep the numbering sequential within a folder.
- Distinguish historical material from fresh captures in both the filename (`historical-…`) and the README.
- Recapture an image when the source, comments, or labels it shows change. Record the recapture date in the folder README.
- Simulator and debugger output stays literal, even when it uses lowercase. Banners and captions added on top follow the uppercase convention in [assembly.md](assembly.md).
- Give every image meaningful alt text that says what the reader should notice, not "screenshot".
- Copy any needed asset from `transcripts/` into a versioned folder; **never** link into `transcripts/`.

## Review focus

Agents that can read images **must** open each changed image, not only its filename.

- Does the image show what the surrounding text claims (register values, addresses, flags, highlighted line)?
- Does the source visible in the image match the current file in `codes/`?
- Is the important region legible at normal page width, and is the crop free of unrelated UI?
- Do the alt text and caption still match after a recapture?

## Decisions

Reviewers **must not** propose changes that reverse these.

<!-- - YYYY-MM-DD: <decision>. Reason: <why>. -->
