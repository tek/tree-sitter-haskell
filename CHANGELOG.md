# Changelog

## Unreleased

### Changed

- `comment` and `haddock` are no longer opaque leaf tokens: when non-empty they now
  expose a `content` child holding the body, with the opening delimiter (`--`,
  `-- |`, `{-`, `{- ^`, …) left as a hidden token. Queries can now target the body
  separately from the delimiter, e.g. `(comment content: (content))`. Note that for
  block comments the closing `-}` is part of `content`.
- `queries/injections.scm`: the `comment` language injection now targets only the
  `content` child, excluding the leading `--`/`{-` delimiter from the injected range.
