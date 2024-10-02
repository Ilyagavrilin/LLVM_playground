; ModuleID = 'app.c'
source_filename = "app.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nounwind uwtable
define dso_local void @draw_circle(i32 noundef %0, i32 noundef %1, i32 noundef %2, i32 noundef %3) local_unnamed_addr #0 {
  %5 = icmp slt i32 %2, 0
  br i1 %5, label %30, label %6

6:                                                ; preds = %4
  %7 = sub nsw i32 1, %2
  br label %8

8:                                                ; preds = %6, %8
  %9 = phi i32 [ %28, %8 ], [ %7, %6 ]
  %10 = phi i32 [ %20, %8 ], [ 0, %6 ]
  %11 = phi i32 [ %23, %8 ], [ %2, %6 ]
  %12 = add nsw i32 %11, %0
  %13 = add nsw i32 %10, %1
  tail call void @simPutPixel(i32 noundef %12, i32 noundef %13, i32 noundef %3) #3
  %14 = add nsw i32 %10, %0
  %15 = add nsw i32 %11, %1
  tail call void @simPutPixel(i32 noundef %14, i32 noundef %15, i32 noundef %3) #3
  %16 = sub nsw i32 %0, %11
  tail call void @simPutPixel(i32 noundef %16, i32 noundef %13, i32 noundef %3) #3
  %17 = sub nsw i32 %0, %10
  tail call void @simPutPixel(i32 noundef %17, i32 noundef %15, i32 noundef %3) #3
  %18 = sub nsw i32 %1, %10
  tail call void @simPutPixel(i32 noundef %16, i32 noundef %18, i32 noundef %3) #3
  %19 = sub nsw i32 %1, %11
  tail call void @simPutPixel(i32 noundef %17, i32 noundef %19, i32 noundef %3) #3
  tail call void @simPutPixel(i32 noundef %12, i32 noundef %18, i32 noundef %3) #3
  tail call void @simPutPixel(i32 noundef %14, i32 noundef %19, i32 noundef %3) #3
  %20 = add nuw nsw i32 %10, 1
  %21 = icmp slt i32 %9, 1
  %22 = add nsw i32 %11, -1
  %23 = select i1 %21, i32 %11, i32 %22
  %24 = select i1 %21, i32 0, i32 %22
  %25 = sub nsw i32 %20, %24
  %26 = shl nsw i32 %25, 1
  %27 = add i32 %9, 1
  %28 = add i32 %27, %26
  %29 = icmp slt i32 %10, %23
  br i1 %29, label %8, label %30, !llvm.loop !5

30:                                               ; preds = %8, %4
  ret void
}

declare void @simPutPixel(i32 noundef, i32 noundef, i32 noundef) local_unnamed_addr #1

; Function Attrs: nounwind uwtable
define dso_local void @draw_rectangle(i32 noundef %0, i32 noundef %1, i32 noundef %2, i32 noundef %3, i32 noundef %4) local_unnamed_addr #0 {
  %6 = add nsw i32 %2, %0
  %7 = icmp sgt i32 %2, 0
  br i1 %7, label %8, label %20

8:                                                ; preds = %5
  %9 = add nsw i32 %3, %1
  %10 = icmp sgt i32 %3, 0
  br i1 %10, label %11, label %20

11:                                               ; preds = %8, %17
  %12 = phi i32 [ %18, %17 ], [ %0, %8 ]
  br label %13

13:                                               ; preds = %11, %13
  %14 = phi i32 [ %1, %11 ], [ %15, %13 ]
  tail call void @simPutPixel(i32 noundef %12, i32 noundef %14, i32 noundef %4) #3
  %15 = add nsw i32 %14, 1
  %16 = icmp slt i32 %15, %9
  br i1 %16, label %13, label %17, !llvm.loop !7

17:                                               ; preds = %13
  %18 = add nsw i32 %12, 1
  %19 = icmp slt i32 %18, %6
  br i1 %19, label %11, label %20, !llvm.loop !8

20:                                               ; preds = %17, %8, %5
  ret void
}

; Function Attrs: noreturn nounwind uwtable
define dso_local void @app() local_unnamed_addr #2 {
  %1 = tail call i32 (...) @simRand() #3
  %2 = srem i32 %1, 600
  %3 = tail call i32 (...) @simRand() #3
  %4 = srem i32 %3, 400
  br label %5

5:                                                ; preds = %120, %0
  %6 = phi i32 [ 2, %0 ], [ %75, %120 ]
  %7 = phi i32 [ 2, %0 ], [ %18, %120 ]
  %8 = phi i32 [ %2, %0 ], [ %55, %120 ]
  %9 = phi i32 [ %4, %0 ], [ %56, %120 ]
  %10 = phi i32 [ 300, %0 ], [ %24, %120 ]
  %11 = phi i32 [ 40, %0 ], [ %58, %120 ]
  %12 = phi i32 [ 400, %0 ], [ %19, %120 ]
  %13 = add nsw i32 %12, %7
  %14 = add nsw i32 %10, %6
  %15 = add i32 %13, -790
  %16 = icmp ult i32 %15, -779
  %17 = sub nsw i32 0, %7
  %18 = select i1 %16, i32 %17, i32 %7
  %19 = select i1 %16, i32 %12, i32 %13
  %20 = add i32 %14, -590
  %21 = icmp ult i32 %20, -579
  %22 = sub nsw i32 0, %6
  %23 = select i1 %21, i32 %22, i32 %6
  %24 = select i1 %21, i32 %10, i32 %14
  %25 = icmp sgt i32 %11, 39
  br i1 %25, label %26, label %54

26:                                               ; preds = %5
  %27 = add nsw i32 %9, 199
  %28 = add nsw i32 %8, 199
  br label %29

29:                                               ; preds = %26, %35
  %30 = phi i32 [ %36, %35 ], [ %8, %26 ]
  br label %31

31:                                               ; preds = %31, %29
  %32 = phi i32 [ %9, %29 ], [ %33, %31 ]
  tail call void @simPutPixel(i32 noundef %30, i32 noundef %32, i32 noundef 0) #3
  %33 = add i32 %32, 1
  %34 = icmp eq i32 %32, %27
  br i1 %34, label %35, label %31, !llvm.loop !7

35:                                               ; preds = %31
  %36 = add i32 %30, 1
  %37 = icmp eq i32 %30, %28
  br i1 %37, label %38, label %29, !llvm.loop !8

38:                                               ; preds = %35
  %39 = tail call i32 (...) @simRand() #3
  %40 = srem i32 %39, 600
  %41 = tail call i32 (...) @simRand() #3
  %42 = srem i32 %41, 400
  %43 = add nsw i32 %42, 199
  %44 = add nsw i32 %40, 199
  br label %45

45:                                               ; preds = %51, %38
  %46 = phi i32 [ %52, %51 ], [ %40, %38 ]
  br label %47

47:                                               ; preds = %47, %45
  %48 = phi i32 [ %42, %45 ], [ %49, %47 ]
  tail call void @simPutPixel(i32 noundef %46, i32 noundef %48, i32 noundef -65536) #3
  %49 = add nsw i32 %48, 1
  %50 = icmp eq i32 %48, %43
  br i1 %50, label %51, label %47, !llvm.loop !7

51:                                               ; preds = %47
  %52 = add nsw i32 %46, 1
  %53 = icmp eq i32 %46, %44
  br i1 %53, label %54, label %45, !llvm.loop !8

54:                                               ; preds = %51, %5
  %55 = phi i32 [ %8, %5 ], [ %40, %51 ]
  %56 = phi i32 [ %9, %5 ], [ %42, %51 ]
  %57 = phi i32 [ %11, %5 ], [ 0, %51 ]
  %58 = add nuw nsw i32 %57, 1
  %59 = add nsw i32 %19, 10
  %60 = icmp sgt i32 %59, %55
  br i1 %60, label %61, label %74

61:                                               ; preds = %54
  %62 = add nsw i32 %19, -10
  %63 = add nsw i32 %55, 200
  %64 = icmp slt i32 %62, %63
  %65 = add nsw i32 %24, 10
  %66 = icmp sgt i32 %65, %56
  %67 = select i1 %64, i1 %66, i1 false
  br i1 %67, label %68, label %74

68:                                               ; preds = %61
  %69 = add nsw i32 %24, -10
  %70 = add nsw i32 %56, 200
  %71 = icmp slt i32 %69, %70
  %72 = sub nsw i32 0, %23
  %73 = select i1 %71, i32 %72, i32 %23
  br label %74

74:                                               ; preds = %68, %61, %54
  %75 = phi i32 [ %23, %61 ], [ %23, %54 ], [ %73, %68 ]
  br label %76

76:                                               ; preds = %76, %74
  %77 = phi i32 [ %96, %76 ], [ -9, %74 ]
  %78 = phi i32 [ %88, %76 ], [ 0, %74 ]
  %79 = phi i32 [ %91, %76 ], [ 10, %74 ]
  %80 = add nsw i32 %79, %12
  %81 = add nsw i32 %78, %10
  tail call void @simPutPixel(i32 noundef %80, i32 noundef %81, i32 noundef 0) #3
  %82 = add nsw i32 %78, %12
  %83 = add nsw i32 %79, %10
  tail call void @simPutPixel(i32 noundef %82, i32 noundef %83, i32 noundef 0) #3
  %84 = sub nsw i32 %12, %79
  tail call void @simPutPixel(i32 noundef %84, i32 noundef %81, i32 noundef 0) #3
  %85 = sub nsw i32 %12, %78
  tail call void @simPutPixel(i32 noundef %85, i32 noundef %83, i32 noundef 0) #3
  %86 = sub nsw i32 %10, %78
  tail call void @simPutPixel(i32 noundef %84, i32 noundef %86, i32 noundef 0) #3
  %87 = sub nsw i32 %10, %79
  tail call void @simPutPixel(i32 noundef %85, i32 noundef %87, i32 noundef 0) #3
  tail call void @simPutPixel(i32 noundef %80, i32 noundef %86, i32 noundef 0) #3
  tail call void @simPutPixel(i32 noundef %82, i32 noundef %87, i32 noundef 0) #3
  %88 = add nuw nsw i32 %78, 1
  %89 = icmp slt i32 %77, 1
  %90 = add nsw i32 %79, -1
  %91 = select i1 %89, i32 %79, i32 %90
  %92 = select i1 %89, i32 0, i32 %90
  %93 = sub nsw i32 %88, %92
  %94 = shl nsw i32 %93, 1
  %95 = add i32 %77, 1
  %96 = add i32 %95, %94
  %97 = icmp slt i32 %78, %91
  br i1 %97, label %76, label %98, !llvm.loop !5

98:                                               ; preds = %76, %98
  %99 = phi i32 [ %118, %98 ], [ -9, %76 ]
  %100 = phi i32 [ %110, %98 ], [ 0, %76 ]
  %101 = phi i32 [ %113, %98 ], [ 10, %76 ]
  %102 = add nsw i32 %101, %19
  %103 = add nsw i32 %100, %24
  tail call void @simPutPixel(i32 noundef %102, i32 noundef %103, i32 noundef -1) #3
  %104 = add nsw i32 %100, %19
  %105 = add nsw i32 %101, %24
  tail call void @simPutPixel(i32 noundef %104, i32 noundef %105, i32 noundef -1) #3
  %106 = sub nsw i32 %19, %101
  tail call void @simPutPixel(i32 noundef %106, i32 noundef %103, i32 noundef -1) #3
  %107 = sub nsw i32 %19, %100
  tail call void @simPutPixel(i32 noundef %107, i32 noundef %105, i32 noundef -1) #3
  %108 = sub nsw i32 %24, %100
  tail call void @simPutPixel(i32 noundef %106, i32 noundef %108, i32 noundef -1) #3
  %109 = sub nsw i32 %24, %101
  tail call void @simPutPixel(i32 noundef %107, i32 noundef %109, i32 noundef -1) #3
  tail call void @simPutPixel(i32 noundef %102, i32 noundef %108, i32 noundef -1) #3
  tail call void @simPutPixel(i32 noundef %104, i32 noundef %109, i32 noundef -1) #3
  %110 = add nuw nsw i32 %100, 1
  %111 = icmp slt i32 %99, 1
  %112 = add nsw i32 %101, -1
  %113 = select i1 %111, i32 %101, i32 %112
  %114 = select i1 %111, i32 0, i32 %112
  %115 = sub nsw i32 %110, %114
  %116 = shl nsw i32 %115, 1
  %117 = add i32 %99, 1
  %118 = add i32 %117, %116
  %119 = icmp slt i32 %100, %113
  br i1 %119, label %98, label %120, !llvm.loop !5

120:                                              ; preds = %98
  tail call void (...) @simFlush() #3
  br label %5, !llvm.loop !9
}

declare i32 @simRand(...) local_unnamed_addr #1

declare void @simFlush(...) local_unnamed_addr #1

attributes #0 = { nounwind uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { noreturn nounwind uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { nounwind }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{!"Ubuntu clang version 18.1.3 (1ubuntu1)"}
!5 = distinct !{!5, !6}
!6 = !{!"llvm.loop.mustprogress"}
!7 = distinct !{!7, !6}
!8 = distinct !{!8, !6}
!9 = distinct !{!9, !6}
