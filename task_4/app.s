; Main function to initiate the drawing loop
app:
    ; Generate initial random x and y coordinates within 600x400 bounds
    CALL SIM_RAND R1                   ; R1 = random number for x
    MOD R1 R1 600                     ; x = R1 % 600
    CALL SIM_RAND R2                   ; R2 = random number for y
    MOD R2 R2 400                     ; y = R2 % 400

    ; Set fixed parameters for drawing
    MOV R3 100                         ; R3 = radius (circle radius = 100)
    MOV R4 16777215                    ; R4 = color (white color = 0xFFFFFF)
    MOV R5 50                          ; R5 = width (rectangle width = 50)
    MOV R6 30                          ; R6 = height (rectangle height = 30)

    ; Initial drawing of circle and rectangle at (x y)
    CALL draw_circle R1 R2 R3 R4    ; draw_circle(x y radius color)
    CALL draw_rectangle R1 R2 R5 R6 R4 ; draw_rectangle(x y width height color)

    ; Flush the canvas to display initial drawing
    CALL SIM_FLUSH

    ; Main loop for continuous drawing and updating
loop:
    ; Generate new random x and y coordinates within 600x400 bounds
    CALL SIM_RAND R1                   ; R1 = random number for new x
    MOD R1 R1 600                     ; newX = R1 % 600
    CALL SIM_RAND R2                   ; R2 = random number for new y
    MOD R2 R2 400                     ; newY = R2 % 400

    ; Draw circle and rectangle at new coordinates
    CALL draw_circle R1 R2 R3 R4    ; draw_circle(newX newY radius color)
    CALL draw_rectangle R1 R2 R5 R6 R4 ; draw_rectangle(newX newY width height color)

    ; Flush the canvas to display updated drawing
    CALL SIM_FLUSH

    ; Loop back to repeat the drawing process
    BR loop


; Function to draw a circle at (x y) with given radius and color
draw_circle:
    ; Prologue: Save registers
    PUSH FP
    MOV FP SP
    PUSH R1  ; Save x
    PUSH R2  ; Save y
    PUSH R3  ; Save radius
    PUSH R4  ; Save color

    ; Check if radius is less than 0 if so return
    MOV R7 0
    CMP R5 R3 R7
    BR_IF R5 end_draw_circle

draw_circle_loop:
    ; Draw circle by calculating symmetrical points
    MOV R6 R1               ; R6 = x
    ADD R6 R6 R3           ; R6 = x + radius
    MOV R7 R2               ; R7 = y

    CALL SIM_PUT_PIXEL R6 R7 R4  ; Draw pixel at (x+radius y)
    
    ; Calculate and draw symmetrical points
    SUB R8 R1 R3           ; R8 = x - radius
    CALL SIM_PUT_PIXEL R8 R7 R4  ; Draw pixel at (x-radius y)

    ADD R9 R2 R3           ; R9 = y + radius
    CALL SIM_PUT_PIXEL R1 R9 R4  ; Draw pixel at (x y+radius)

    SUB R10 R2 R3          ; R10 = y - radius
    CALL SIM_PUT_PIXEL R1 R10 R4 ; Draw pixel at (x y-radius)

    ; Update radius and loop condition
    ADD R3 R3 -1            ; radius -= 1
    MOV R7 0
    CMP R5 R3 R7             ; Check if radius >= 0
    BR_IF R5 draw_circle_loop

end_draw_circle:
    ; Epilogue: Restore registers and return
    POP R4  ; Restore color
    POP R3  ; Restore radius
    POP R2  ; Restore y
    POP R1  ; Restore x
    POP FP
    RET


; Function to draw a rectangle at (x y) with specified width height and color
draw_rectangle:
    ; Prologue: Save registers
    PUSH FP
    MOV FP SP
    PUSH R1  ; Save x
    PUSH R2  ; Save y
    PUSH R3  ; Save width
    PUSH R4  ; Save height
    PUSH R5  ; Save color

    ; Calculate rectangle bounds
    ADD R6 R1 R3            ; R6 = x + width (right bound)
    ADD R7 R2 R4            ; R7 = y + height (bottom bound)

    ; Loop over each row (y-axis)
rectangle_height_loop:
    MOV R8 R1                ; Start from left bound R8 = current x position
rectangle_width_loop:
    ; Draw pixel at (R8 R2)
    CALL SIM_PUT_PIXEL R8 R2 R5

    ; Move to the next pixel in the row
    ADD R8 R8 1             ; R8 = R8 + 1
    CMP R9 R8 R6            ; Check if we've reached the right bound
    BR_IF R9 rectangle_width_loop   ; Continue drawing row

    ; Move to the next row
    ADD R2 R2 1             ; R2 = R2 + 1 (increment y position)
    CMP R9 R2 R7            ; Check if we've reached the bottom bound
    BR_IF R9 rectangle_height_loop  ; Continue drawing height

    ; Epilogue: Restore registers and return
    POP R5  ; Restore color
    POP R4  ; Restore height
    POP R3  ; Restore width
    POP R2  ; Restore y
    POP R1  ; Restore x
    POP FP
    RET