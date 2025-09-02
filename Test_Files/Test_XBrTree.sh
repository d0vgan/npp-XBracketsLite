[ ' "# 123 ]
[ ' ] "# 123 ]
[ ' ] "#' 123 ]
[ ' " ]# 123 ]
[ ' " ]# "123 ]
[ ' "# 123' ]
[ ' "# 123" ]
[ ' "# 123" ' ]
[ ' "# 123' " ]

if {[catch {

# Argument $tname
# brackets pairs: { [db] }.
# only left bracket: [
# only right bracket: ]
proc is_without_rowid {tname} {
  set t [string map {' ''} $tname]
  db eval "PRAGMA index_list = '$t'" o {
    if {$o(origin) == "pk"} {
      set n $o(name)
      if {0==[db one { SELECT count(*) FROM sqlite_schema WHERE name=$n }]} {
        return 1
      }
    }
  }
  return 0
}

repo_info="$(git rev-parse --git-dir --is-inside-git-dir \
		--is-bare-repository --is-inside-work-tree \
		--short HEAD 2>/dev/null)"

}