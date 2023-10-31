; ModuleID = 'test1.c'
source_filename = "test1.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx13.0.0"

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @main() #0 {
entry:
  %retval = alloca i32, align 4
  %i = alloca i32, align 4
  %A = alloca [20 x i32], align 4
  %B = alloca [20 x i32], align 4
  %C = alloca [20 x i32], align 4
  store i32 0, ptr %retval, align 4
  store i32 4, ptr %i, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i32, ptr %i, align 4
  %cmp = icmp slt i32 %0, 20
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %1 = load i32, ptr %i, align 4
  %idxprom = sext i32 %1 to i64
  %arrayidx = getelementptr inbounds [20 x i32], ptr %C, i64 0, i64 %idxprom
  %2 = load i32, ptr %arrayidx, align 4
  %3 = load i32, ptr %i, align 4
  %idxprom1 = sext i32 %3 to i64
  %arrayidx2 = getelementptr inbounds [20 x i32], ptr %A, i64 0, i64 %idxprom1
  store i32 %2, ptr %arrayidx2, align 4
  %4 = load i32, ptr %i, align 4
  %sub = sub nsw i32 %4, 4
  %idxprom3 = sext i32 %sub to i64
  %arrayidx4 = getelementptr inbounds [20 x i32], ptr %A, i64 0, i64 %idxprom3
  %5 = load i32, ptr %arrayidx4, align 4
  %6 = load i32, ptr %i, align 4
  %idxprom5 = sext i32 %6 to i64
  %arrayidx6 = getelementptr inbounds [20 x i32], ptr %B, i64 0, i64 %idxprom5
  store i32 %5, ptr %arrayidx6, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %7 = load i32, ptr %i, align 4
  %inc = add nsw i32 %7, 1
  store i32 %inc, ptr %i, align 4
  br label %for.cond, !llvm.loop !5

for.end:                                          ; preds = %for.cond
  ret i32 0
}

attributes #0 = { noinline nounwind ssp uwtable(sync) "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="apple-m1" "target-features"="+aes,+crc,+dotprod,+fp-armv8,+fp16fml,+fullfp16,+lse,+neon,+ras,+rcpc,+rdm,+sha2,+sha3,+v8.1a,+v8.2a,+v8.3a,+v8.4a,+v8.5a,+v8a,+zcm,+zcz" }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"uwtable", i32 1}
!3 = !{i32 7, !"frame-pointer", i32 1}
!4 = !{!"clang version 17.0.2"}
!5 = distinct !{!5, !6}
!6 = !{!"llvm.loop.mustprogress"}
