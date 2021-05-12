# 3rd Party Tool: PackCC

This deeply beautiful piece of software is a packrat parser generator for C,
supporting left recursion, written by Arihiro Yoshida. The file packcc.c in
this directory is covered by the license LICENSE in this directory.  It is
maintained in the repo at https://github.com/arithy/packcc, but I've included
it directly in this project because:

- The license is compatible.
- The entire thing is one C file with no library dependencies or complicated
  build steps.
- It is inexplicably not available everywhere by default.

It is not my intention to change this source in any way to diverge from
upstream, so periodically fresh versions from upstream will be imported
into this repo.
