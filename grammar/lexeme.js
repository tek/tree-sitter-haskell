const {
  id_char,
  varid_start_char,
  conid_start_char,
} = require('./util.js')

module.exports = {

  variable: _ => token(seq(varid_start_char, id_char, /#*/)),

  implicit_variable: _ => token(seq('?', varid_start_char, id_char)),

  name: _ => token(seq(conid_start_char, id_char, /#*/)),

  label: _ => token(seq('#', varid_start_char, id_char)),

  _carrow: _ => choice('=>', '⇒'),
  _arrow: _ => choice('->', '→'),
  _linear_arrow: _ => choice('->.', '⊸'),
  _larrow: _ => choice('<-', '←'),
  _colon2: _ => choice('::', '∷'),
  _promote: _ => '\'',

  _qual_dot: $ => seq($._cond_qual_dot, '.'),
  _tight_dot: $ => seq($._cond_tight_dot, '.'),
  _any_tight_dot: $ => choice($._qual_dot, $._tight_dot),
  _prefix_dot: $ => seq($._cond_prefix_dot, '.'),
  _any_prefix_dot: $ => choice($._qual_dot, $._prefix_dot),

  _tight_at: $ => seq($._cond_tight_at, '@'),
  _prefix_at: $ => seq($._cond_prefix_at, '@'),

  _prefix_bang: $ => seq($._cond_prefix_bang, '!'),
  _tight_bang: $ => seq($._cond_tight_bang, '!'),
  _any_prefix_bang: $ => choice($._prefix_bang, $._tight_bang),

  _prefix_tilde: $ => seq($._cond_prefix_tilde, '~'),
  _tight_tilde: $ => seq($._cond_tight_tilde, '~'),
  _any_prefix_tilde: $ => choice($._prefix_tilde, $._tight_tilde),

  _prefix_percent: $ => seq($._cond_prefix_percent, '%'),

  _dotdot: $ => seq($._cond_dotdot, '..'),

  _paren_open: $ => seq(alias(/\(/, '('), $._cmd_texp_start),
  _paren_close: $ => seq(alias(/\)/, ')'), $._cmd_texp_end),
  _bracket_open: $ => seq('[', $._cmd_texp_start),
  _bracket_close: $ => seq(']', $._cmd_texp_end),

  // Sadly, this does not have the effect of creating a single terminal for the bracket :'(
  _unboxed_open: $ => alias(seq($._paren_open, token.immediate('#')), '(#'),
  _unboxed_close: $ => seq('#)', $._cmd_texp_end),
  _unboxed_bar: _ => choice('|', token.immediate('|')),

  _where: $ => seq(optional($._phantom_where), 'where'),

  _bar: $ => seq(optional($._phantom_bar), '|'),

  // The node types aliased here (`marker`/`content`, not to be confused with the fields of the
  // same name) are deliberately named without the substring "comment": the upstream `tree-sitter
  // test` highlighting-assertion parser identifies which nodes to scan for `-- <- capture`-style
  // assertions by checking `node.kind().contains("comment")`, so a child type containing that
  // substring gets misparsed as its own assertion comment (its content, e.g. `" <- module"`,
  // recontains a spurious arrow), producing a bogus phantom assertion at the wrong column.
  comment: $ => choice(
    seq(
      field('marker', alias($._comment_marker, $.marker)),
      field('content', alias($._comment_text, $.content)),
    ),
    field('marker', alias($._comment_marker_only, $.marker)),
  ),

  haddock: $ => choice(
    seq(
      field('marker', alias($._haddock_marker, $.marker)),
      field('content', alias($._comment_text, $.content)),
    ),
    field('marker', alias($._haddock_marker_only, $.marker)),
  ),

}
