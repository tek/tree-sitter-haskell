use tree_sitter::{Parser, Tree, TreeCursor};

fn print_tree(src: &str, tree: &Tree) -> String {
    let mut cursor = tree.walk();
    return print_cursor(src, &mut cursor, 0);
}

fn print_cursor(src: &str, cursor: &mut TreeCursor, depth: usize) -> String {
    let mut result = String::from("");
    loop {
        let node = cursor.node();
        node.end_position();

        let formatted_node = format!(
            "{} {} - {}",
            node.kind().replace('\n', "\\n"),
            node.start_position(),
            node.end_position()
        );

        if node.child_count() == 0 {
            let node_src = &src[node.start_byte()..node.end_byte()];
            result += &(format!("{}{} {:?}\n", "  ".repeat(depth), formatted_node, node_src));
        } else {
            result += &(format!("{}{}\n", "  ".repeat(depth), formatted_node,));
        }

        if cursor.goto_first_child() {
            result += &print_cursor(src, cursor, depth + 1);
            cursor.goto_parent();
        }

        if !cursor.goto_next_sibling() {
            break;
        }
    }
    return result;
}

fn check_tree(parser: &mut Parser, target: &str, code: &str) {
    let tree = parser.parse(code, None).unwrap();
    assert_eq!(target, print_tree(code, &tree));
}

#[test]
fn parse_tests() {
    let mut parser = Parser::new();
    parser.set_language(tree_sitter_haskell::language()).expect("Error loading grammar");

    let mut check = |target: &str, code: &str| { check_tree(&mut parser, target, code); };

    let code1 = "import A.A.A";

    let target1: &str = r#"haskell (0, 0) - (0, 12)
  imports (0, 0) - (0, 12)
    import (0, 0) - (0, 12)
      import (0, 0) - (0, 6) "import"
      qualified_module (0, 7) - (0, 12)
        module (0, 7) - (0, 8) "A"
        . (0, 8) - (0, 9) "."
        module (0, 9) - (0, 10) "A"
        . (0, 10) - (0, 11) "."
        module (0, 11) - (0, 12) "A"
"#;

    check(target1, code1);

    let code2 = "(.===) :: A";

    let target2 = r#"haskell (0, 0) - (0, 11)
  declarations (0, 0) - (0, 11)
    signature (0, 0) - (0, 11)
      ( (0, 0) - (0, 1) "("
      operator (0, 1) - (0, 5) ".==="
      ) (0, 5) - (0, 6) ")"
      :: (0, 7) - (0, 9) "::"
      type_name (0, 10) - (0, 11)
        type (0, 10) - (0, 11) "A"
"#;

    check(target2, code2);

    let code3 = "a = a ++ a";

    let target3 = r#"haskell (0, 0) - (0, 10)
  declarations (0, 0) - (0, 10)
    function (0, 0) - (0, 10)
      variable (0, 0) - (0, 1) "a"
      = (0, 2) - (0, 3) "="
      exp_infix (0, 4) - (0, 10)
        exp_name (0, 4) - (0, 5)
          variable (0, 4) - (0, 5) "a"
        operator (0, 6) - (0, 8) "++"
        exp_name (0, 9) - (0, 10)
          variable (0, 9) - (0, 10) "a"
"#;

    check(target3, code3);

    let code4 = "(#|) :: A";

    // TODO some change has ruined this, creating now two nodes for the operator.
    // Investigate with bisection.
    let target4 = r##"haskell (0, 0) - (0, 9)
  declarations (0, 0) - (0, 9)
    signature (0, 0) - (0, 9)
      ( (0, 0) - (0, 1) "("
      operator (0, 1) - (0, 3) "#|"
      ) (0, 3) - (0, 4) ")"
      :: (0, 5) - (0, 7) "::"
      type_name (0, 8) - (0, 9)
        type (0, 8) - (0, 9) "A"
"##;

    check(target4, code4);

    let code5 = "(|#) :: A";

    let target5 = r##"haskell (0, 0) - (0, 9)
  declarations (0, 0) - (0, 9)
    signature (0, 0) - (0, 9)
      ( (0, 0) - (0, 1) "("
      operator (0, 1) - (0, 3) "|#"
      ) (0, 3) - (0, 4) ")"
      :: (0, 5) - (0, 7) "::"
      type_name (0, 8) - (0, 9)
        type (0, 8) - (0, 9) "A"
"##;

    check(target5, code5);

    let code6 = "(#) :: A";

    let target6 = r##"haskell (0, 0) - (0, 8)
  declarations (0, 0) - (0, 8)
    signature (0, 0) - (0, 8)
      ( (0, 0) - (0, 1) "("
      operator (0, 1) - (0, 2) "#"
      ) (0, 2) - (0, 3) ")"
      :: (0, 4) - (0, 6) "::"
      type_name (0, 7) - (0, 8)
        type (0, 7) - (0, 8) "A"
"##;

    check(target6, code6);

    let code7 = "a = a ## a";

    let target7 = r###"haskell (0, 0) - (0, 10)
  declarations (0, 0) - (0, 10)
    function (0, 0) - (0, 10)
      variable (0, 0) - (0, 1) "a"
      = (0, 2) - (0, 3) "="
      exp_infix (0, 4) - (0, 10)
        exp_name (0, 4) - (0, 5)
          variable (0, 4) - (0, 5) "a"
        operator (0, 6) - (0, 8) "##"
        exp_name (0, 9) - (0, 10)
          variable (0, 9) - (0, 10) "a"
"###;

    check(target7, code7);

    let code8 = "a = (# 1, 2, #)";

    let target8 = r###"haskell (0, 0) - (0, 15)
  declarations (0, 0) - (0, 15)
    function (0, 0) - (0, 15)
      variable (0, 0) - (0, 1) "a"
      = (0, 2) - (0, 3) "="
      exp_unboxed_tuple (0, 4) - (0, 15)
        (# (0, 4) - (0, 5)
          ( (0, 4) - (0, 5) "("
        (# (0, 5) - (0, 6) "#"
        exp_literal (0, 7) - (0, 8)
          integer (0, 7) - (0, 8) "1"
        , (0, 8) - (0, 9) ","
        exp_literal (0, 10) - (0, 11)
          integer (0, 10) - (0, 11) "2"
        , (0, 11) - (0, 12) ","
        #) (0, 13) - (0, 15) "#)"
"###;

    check(target8, code8);
}
