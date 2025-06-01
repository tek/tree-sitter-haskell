(
  (
    (haddock) @doc
    .
    [

      (header (module) @name) @definition.module

      (declarations
        .
        [
          (function name: (variable) @name) @definition.function
          (signature name: (variable) @name) @definition.function
          (bind name: (variable) @name) @definition.variable
          (data_type name: (name) @name) @definition.class
          (newtype name: (name) @name) @definition.class
          (class name: (name) @name) @definition.interface
          (instance name: (name) @name) @reference.implementation
        ]
      )

      (class_declarations
        .
        [
          (function name: (variable) @name)
          (signature name: (variable) @name)
          (bind name: (variable) @name)
        ] @definition.method
      )

      (instance_declarations
        .
        [
          (function name: (variable) @name)
          (signature name: (variable) @name)
          (bind name: (variable) @name)
        ] @definition.method
      )

    ]
  )
  (#strip! @doc "^\s*--+\\s*(\\|\\s*)?")
  (#select-adjacent! @doc @definition.function)
 )

(
  [

    (declarations
      (haddock) @doc
      .
      [
        (function name: (variable) @name) @definition.function
        (signature name: (variable) @name) @definition.function
        (bind name: (variable) @name) @definition.variable
        (data_type name: (name) @name) @definition.class
        (newtype name: (name) @name) @definition.class
        (class name: (name) @name) @definition.interface
        (instance name: (name) @name) @reference.implementation
      ]
    )

    (class_declarations
      (haddock)? @doc
      .
      [
        (signature name: (variable) @name)
        (function name: (variable) @name)
        (bind name: (variable) @name)
      ] @definition.method
    )

    (instance_declarations
      (haddock)? @doc
      .
      [
        (signature name: (variable) @name)
        (function name: (variable) @name)
        (bind name: (variable) @name)
      ] @definition.method
    )

   ]
  (#strip! @doc "^\s*--+\\s*(\\|\\s*)?")
  (#select-adjacent! @doc @definition.function)
)

(declarations
  [
    (function name: (variable) @name) @definition.function
    (signature name: (variable) @name) @definition.function
    (bind name: (variable) @name) @definition.variable
  ]
)

(header (module) @name) @definition.module

(data_type name: (name) @name) @definition.class

(newtype name: (name) @name) @definition.class

(pattern/variable) @definition.variable

(expression/variable) @reference.variable

(expression/apply) @reference.call
