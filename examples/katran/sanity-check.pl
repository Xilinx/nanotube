###########################################################################
## Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
## SPDX-License-Identifier: MIT
############################################################################

while (<>) {
  next unless /^define.*\@balancer_ingress_nt\(/../^\}/;
  next if /^ *switch .*\[$/../^ *\]$/;
  next if /^ *(br|unreachable)( |,|$)/;
  next if /^[\w\._]+:|^$/;
  next if /^ *ret void$/;

  next if /^ *%[\w\._]+ = (add|and|sub|mul|urem|xor|or|shl|lshr)( nuw)?( nsw)?( exact)? i\d+ (%[\w\._]+|\d+)/;
  next if /^ *%[\w\._]+ = (zext|trunc|select) i\d+ (%[\w\._]+|\d+)/;
  next if /^ *%[\w\._]+ = icmp (eq|ne|ult|ugt|slt|sgt) i\d+ (%[\w\._]+|\d+)/;
  next if /^ *%[\w\._]+ = phi i\d+ \[/;

  next if /^ *%[\w\._]+ = add \<\d+ x i\d+\> %[\w\._]+,/;
  next if /^ *%[\w\._]+ = insertelement \<\d+ x i\d+\> /;

  next if /^ *%[\w\._]+ = alloca (i\d+|%[\w\._]+)(,|$)/;
  next if /^ *%[\w\._]+ = alloca \<\d+ x i\d+\>(,|$)/;
  next if /^ *%[\w\._]+ = alloca \[\d+ x i\d+\](,|$)/;

  next if /^ *%[\w\._]+ = load i\d+,/;
  next if /^ *%[\w\._]+ = load \<\d+ x i\d+\>,/;
  next if /^ *store i\d+ (-?\d+|%[\w\._]+),/;
  next if /^ *store \<\d+ x i\d+\> (\d+|%[\w\._]+),/;

  next if /^ *%[\w\._]+ = phi (i\d+|%[\w\._]+)\* (\[ *%[\w\._]+, %[\w\._]+ *\]\s*(,\s*|$))*/;

  next if /^ *%[\w\._]+ = bitcast i\d+\* %[\w\._]+ to i\d+\*$/;
  next if /^ *%[\w\._]+ = bitcast %[\w\._]+\* %[\w\._]+ to i\d+\*$/;
  next if /^ *%[\w\._]+ = bitcast \<\d+ x i\d+\>\* %[\w\. ]+ to i\d+\*$/;
  next if /^ *%[\w\._]+ = bitcast \[\d+ x i\d+\]\* %[\w\._]+ to i\d+\*$/;
  next if /^ *%[\w\._]+ = bitcast %[\w\._]+\* %[\w\._]+ to \[\d+ x i\d+\]\*$/;

  next if /^ *%[\w\._]+ = getelementptr inbounds (i\d+|%[\w\. ]+), (i\d+|%[\w\. ]+)\* %[\w\. ]+(, i\d+ \d+)+$/;
  next if /^ *%[\w\._]+ = getelementptr inbounds \[\d+ x i\d+\], \[\d+ x i\d+\]\* %[\w\. ]+(, i\d+ \d+)+$/;
  next if /^ *%[\w\._]+ = getelementptr inbounds \([^\(\)]+\), \([^\(\)]+\)\* %[\w\. ]+(, i\d+ \d+)+$/;

  next if /^ *%[\w\._]+ = call i8\* \@llvm\.stacksave\(/;
  next if /^ *%[\w\._]+ = (tail )?call i\d+ \@llvm\.bswap\.i\d+\(/;
  next if /^ *call void \@llvm.dbg.value\(/;
  next if /^ *call void \@llvm.(lifetime\.(start|end)\.p\d+i\d+|stackrestore|dbg\.value)\(/;
  next if /^ *call void \@llvm\.memset\.p0i8\.i64\(i8\* nonnull( align \d+)? %[\w\._]+, i8 -?\d+, i64 \d+, i1 false\)/;
  next if /^ *call void \@llvm\.memcpy\.p0i8\.p0i8\.i64\(i8\*( nonnull)?( align \d+)? %[\w\._]+, i8\*( nonnull)?( align \d+)? %[\w\._]+, i64 \d+, i1 false\)/;

  next if /^ *%[\w\._]+ = call i64 \@nanotube_get_time_ns\(/;
  next if /^ *%[\w\._]+ = call %struct\.nanotube_packet\* \@nanotube_packet_adjust_head\(%struct\.nanotube_packet\* %packet, i32 -?\d+\)/;
  next if /^ *%[\w\._]+ = call i64 \@nanotube_packet_bounded_length\(%struct\.nanotube_packet\* %packet, i64 \d+\)/;
  next if /^ *%[\w\._]+ = call i64 \@nanotube_packet_read\(%struct\.nanotube_packet\* %packet, i8\* %[\w\._]+, i64 (\d+|%[\w\._]+), i64 \d+\)/;
  next if /^ *%[\w\._]+ = call i64 \@nanotube_packet_write_masked\(%struct\.nanotube_packet\* %packet, i8\* %[\w\._]+, i8\* nonnull %[\w\._]+, i64 \d+, i64 \d+\)/;
  next if /^ *call void \@nanotube_packet_set_port\(%struct\.nanotube_packet\* %packet, i8 zeroext (-?\d+|%[\w\._])\)/;

  next if /^ *%[\w\._]+ = icmp eq %struct\.nanotube_packet\* %[\w\._]+, (null|%packet),/;

  next if /^ *call void \@__assert_fail\(/;

  # 0=READ
  next if /^ *%[\w\._]+ = call i64 \@nanotube_map_op\(%struct\.nanotube_context\* %nt_ctx, i16 zeroext %[\w\._]+, i32 0, i8\* %[\w\._]+, i64 \d+, i8\* null, i8\* %[\w\._]+, i8\* null, i64 (\d+|%[\w\._]+), i64 \d+\)/;

  # 3=WRITE
  next if /^ *%[\w\._]+ = call i64 \@nanotube_map_op\(%struct\.nanotube_context\* %nt_ctx, i16 zeroext %[\w\._]+, i32 3, i8\* %[\w\._]+, i64 \d+, i8\* %[\w\._]+, i8\* null, i8\*( nonnull)? %[\w\._]+, i64 (\d+|%[\w\._]+), i64 \d+\)/;

  # Yuck!
  next if /^ *%[\w\._]+ = call i64 \@nanotube_map_op\(%struct\.nanotube_context\* %nt_ctx, i16 zeroext %[\w\._]+, i32 3, i8\* %[\w\._]+, i64 \d+, i8\* %[\w\._]+, i8\* null, i8\* getelementptr inbounds \(\[\d+ x i8\], \[\d+ x i8\]\* \@onemask\d+, i32 \d+, i32 \d+\), i64 0, i64 \d+\)/;

  print;
}
