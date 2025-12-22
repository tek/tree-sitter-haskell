const haskell = require("../grammar.js")

module.exports = grammar(haskell, {
  name: 'hsc',

  externals: ($, previous) => previous,

  supertypes: ($, previous) => previous,

  precedences: ($, previous) => previous,

  conflicts: ($, previous) => previous,

  inline: ($, previous) => previous,

  rules: {
    haskell: ($, previous) => previous,
  },

})
