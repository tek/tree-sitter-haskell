#include "../../src/scanner.c"

void *tree_sitter_hsc_external_scanner_create() {
  return tree_sitter_haskell_external_scanner_create();
}

bool tree_sitter_hsc_external_scanner_scan(void *payload, TSLexer *lexer, const bool *valid_symbols) {
  return tree_sitter_haskell_external_scanner_scan(payload, lexer, valid_symbols);
}

unsigned tree_sitter_hsc_external_scanner_serialize(void *payload, char *buffer) {
  return tree_sitter_haskell_external_scanner_serialize(payload, buffer);
}

void tree_sitter_hsc_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
  return tree_sitter_haskell_external_scanner_deserialize(payload, buffer, length);
}

void tree_sitter_hsc_external_scanner_destroy(void *payload) {
  return tree_sitter_haskell_external_scanner_destroy(payload);
}
