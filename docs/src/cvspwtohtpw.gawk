/^[ \t]*#/ { next; }
/^[ \t]*$/ { next; }
 {
    print "INSERT INTO users (login,name,email,comment) VALUES ('"$1"','"$3"','"$4"','"$5"')"
    print "\\g"
}
