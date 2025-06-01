a = 1

-- | Documentation
fun :: Maybe Int -> String -> IO Bool
fun (Just i) s =
  if i == 1
  then True
  else s == ""

-- | Documentation
fun = 1

-- Comment
bind :: Int
bind = 1
