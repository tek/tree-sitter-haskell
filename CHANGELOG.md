# Changelog

## Unreleased

### Changed

- `comment` and `haddock` are no longer opaque leaf tokens but compound nodes with
  a `marker` child (`--`, `-- |`, `{-`, `{- ^`, …) and, when the comment is
  non-empty, a `content` child holding the body. Their text still concatenates back
  to the exact original span. Queries can now target the body separately from the
  delimiter, e.g. `(comment content: (content))`. Note that for block comments the
  closing `-}` is part of `content`, not `marker`.
- `queries/injections.scm`: the `comment` language injection now targets only the
  `content` child, excluding the leading `--`/`{-` marker from the injected range.
