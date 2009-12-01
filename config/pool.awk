/^#/ {
  next
}

/^[A-Z_]+:$/ {
  collection = substr($1, 1, length($1) - 1)
  next
}

/^$/ {
  collection = ""
  next
}

{
  if ( collection ) print $0 >collection
}
