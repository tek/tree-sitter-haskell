module.exports = {

  comment: $ => seq(
    $._cond_comment,
    choice(
      $._comment_block,
      $._comment_inline,
    ),
  ),

  haddock: $ => seq(
    $._cond_comment,
    choice(
      $._comment_block_haddock,
      $._comment_inline_haddock,
    ),
  ),

  _comment_block_haddock: $ => seq(
    $._comment_start_block,
    choice($._bar, '^'),
    $.content,
    $._comment_end_block,
  ),

  _comment_block: $ => seq(
    $._comment_start_block,
    $.content,
    $._comment_end_block,
  ),

  _comment_start_block: $ => '{-',

  _comment_end_block: $ => '-}',

  _comment_inline_haddock: $ => seq(
    $._comment_start_inline,
    choice($._bar, '^'),
    $.content,
  ),

  _comment_inline: $ => seq(
    $._comment_start_inline,
    $.content,
  ),

  _comment_start_inline: $ => /--+/,

}
