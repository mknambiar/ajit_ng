.section .text.pagetablesetup
.weak page_table_setup
.global page_table_setup
page_table_setup:
  retl
  nop
.weak set_context_table_pointer
.global set_context_table_pointer
set_context_table_pointer:
  retl
  nop
