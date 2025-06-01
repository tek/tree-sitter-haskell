-- | Documentation fun1
fun1 :: Maybe Int -> String -> IO Bool
fun1 (Just i) s =
  if i == 1
  then True
  else s == ""

-- | Documentation fun2
fun2 :: a -> a
fun2 a = a

-- | Documentation equation
equation a = 1

-- | Documentation bind1
bind1 :: Int
bind1 = 1

-- Comment
bind2 :: Int
bind2 = 1
