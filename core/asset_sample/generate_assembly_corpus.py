import os

def generate_corpus():
    blocks = []
    
    registers_64 = ["rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"]
    registers_32 = ["eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp", "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d"]
    registers_16 = ["ax", "bx", "cx", "dx", "si", "di", "bp", "sp", "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w"]
    registers_8  = ["al", "bl", "cl", "dl", "sil", "dil", "bpl", "spl", "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b"]

    # 1. Zeroing Idioms
    for r in registers_64:
        blocks.append(f"""<|concept_start|>
Zeroing 64-bit register {r} via XOR.
<|code|>
; [INTENT] Reset register {r} to zero
xor {r}, {r}
<|rule_register|>
<|commentary|>
The XOR instruction executed on {r} against itself clears all 64 bits to zero.
Microarchitectural register renaming eliminates execution pipeline latency for this zeroing pattern.
<|concept_end|>""")

    # 2. 32-bit Zero-Extension
    for r32, r64 in zip(registers_32, registers_64):
        blocks.append(f"""<|concept_start|>
Zero-extension when writing 32-bit register {r32}.
<|code|>
; [INTENT] Clear {r64} via 32-bit zeroing
xor {r32}, {r32}
<|rule_register|>
<|commentary|>
Writing to 32-bit register {r32} automatically clears the upper 32 bits of {r64}.
This produces a compact two-byte encoding while completely clearing the 64-bit register.
<|concept_end|>""")

    # 3. Arithmetic operations across registers
    arith_ops = [
        ("add", "adds", "Updates Zero, Sign, Carry, and Overflow flags based on sum."),
        ("sub", "subtracts", "Updates Zero, Sign, Carry, and Overflow flags based on difference."),
        ("and", "performs bitwise logical AND on", "Clears Carry and Overflow flags, updates Zero and Sign flags."),
        ("or", "performs bitwise logical OR on", "Clears Carry and Overflow flags, updates Zero and Sign flags."),
        ("xor", "performs bitwise exclusive OR on", "Clears Carry and Overflow flags, updates Zero and Sign flags.")
    ]
    for op, desc, flag_info in arith_ops:
        for i in range(len(registers_64)):
            r_dst = registers_64[i]
            r_src = registers_64[(i + 1) % len(registers_64)]
            blocks.append(f"""<|concept_start|>
Executing {op.upper()} between {r_dst} and {r_src}.
<|code|>
; [INTENT] {op.capitalize()} {r_src} into {r_dst}
{op} {r_dst}, {r_src}
<|rule_register|>
<|commentary|>
Instruction {op} {desc} {r_src} and destination {r_dst}, storing the result in {r_dst}.
{flag_info}
<|concept_end|>""")

    # 4. Immediate assignments
    immediates = [0, 1, 2, 4, 8, 16, 32, 64, 100, 128, 255, 256, 512, 1024, 4096]
    for r in ["rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11"]:
        for imm in immediates:
            blocks.append(f"""<|concept_start|>
Assigning scalar immediate constant {imm} to register {r}.
<|code|>
; [INTENT] Load immediate {imm} into {r}
mov {r}, {imm}
<|rule_register|>
<|commentary|>
Loads the constant value {imm} directly into destination register {r}.
The destination is specified on the left and the source value is on the right.
<|concept_end|>""")

    # 5. Increment and Decrement
    for r in registers_64:
        blocks.append(f"""<|concept_start|>
Incrementing register {r} by one.
<|code|>
; [INTENT] Advance counter {r}
inc {r}
<|rule_register|>
<|commentary|>
Increments destination register {r} by one while preserving the Carry Flag.
Updates the Zero, Sign, and Overflow flags in EFLAGS.
<|concept_end|>""")
        blocks.append(f"""<|concept_start|>
Decrementing register {r} by one.
<|code|>
; [INTENT] Decrement counter {r}
dec {r}
<|rule_register|>
<|commentary|>
Decrements destination register {r} by one while preserving the Carry Flag.
Useful for loop countdown termination checks.
<|concept_end|>""")

    # 6. Shifts & Rotations
    shifts = [1, 2, 3, 4, 8, 16, 32]
    for r in ["rax", "rbx", "rcx", "rdx", "rsi", "rdi"]:
        for s in shifts:
            blocks.append(f"""<|concept_start|>
Fast multiplication by power of two using shift left by {s}.
<|code|>
; [INTENT] Multiply {r} by {1 << s}
shl {r}, {s}
<|rule_register|>
<|commentary|>
Shifts bits in {r} left by {s} positions, multiplying the value by {1 << s}.
Zeros are inserted into the vacated lower bit positions.
<|concept_end|>""")
            blocks.append(f"""<|concept_start|>
Unsigned integer division by power of two using shift right by {s}.
<|code|>
; [INTENT] Divide {r} by {1 << s}
shr {r}, {s}
<|rule_register|>
<|commentary|>
Shifts bits in {r} right by {s} positions, dividing the unsigned value by {1 << s}.
Zeros are inserted into the vacated upper bit positions.
<|concept_end|>""")
            blocks.append(f"""<|concept_start|>
Signed arithmetic division by power of two using shift right by {s}.
<|code|>
; [INTENT] Signed divide {r} by {1 << s}
sar {r}, {s}
<|rule_register|>
<|commentary|>
Shifts bits in {r} right by {s} positions while preserving the sign bit.
Computes a mathematically sound two's complement division by {1 << s}.
<|concept_end|>""")

    # 7. Stack Push / Pop
    for r in registers_64:
        blocks.append(f"""<|concept_start|>
Pushing register {r} to hardware stack.
<|code|>
; [INTENT] Preserve {r} on stack
push {r}
<|rule_memory|>
<|commentary|>
Decrements RSP by 8 bytes and stores the 64-bit value of {r} at address [RSP].
The x86_64 stack grows downwards toward lower addresses.
<|concept_end|>""")
        blocks.append(f"""<|concept_start|>
Popping register {r} from hardware stack.
<|code|>
; [INTENT] Restore {r} from stack
pop {r}
<|rule_memory|>
<|commentary|>
Reads the 64-bit value from address [RSP] into {r} and increments RSP by 8 bytes.
Restores the previously saved register state.
<|concept_end|>""")

    # 8. Local Variable Displacements
    offsets = [8, 16, 24, 32, 40, 48, 64, 80, 96, 128]
    for off in offsets:
        blocks.append(f"""<|concept_start|>
Reading local stack variable at offset negative {off}.
<|code|>
; [INTENT] Read local variable at [rbp - {off}]
mov rax, [rbp - {off}]
<|rule_memory|>
<|commentary|>
Dereferences memory at address RBP minus {off} bytes, loading the 64-bit local variable into RAX.
Negative displacements from RBP access local variables in standard stack frames.
<|concept_end|>""")
        blocks.append(f"""<|concept_start|>
Writing local stack variable at offset negative {off}.
<|code|>
; [INTENT] Store value to local variable at [rbp - {off}]
mov [rbp - {off}], rax
<|rule_memory|>
<|commentary|>
Stores the 64-bit value in RAX into the stack memory location at RBP minus {off}.
Ensures persistent local storage within the function scope.
<|concept_end|>""")

    # 9. SIB Array Scaling
    scales = [1, 2, 4, 8]
    for s in scales:
        for idx_reg in ["rcx", "rsi", "rdx", "rax", "r8", "r9"]:
            blocks.append(f"""<|concept_start|>
Array element access using {idx_reg} scaled by {s}.
<|code|>
; [INTENT] Access element index {idx_reg} in array rdi with element size {s}
mov rax, [rdi + {idx_reg}*{s}]
<|rule_memory|>
<|commentary|>
Computes target address RDI + ({idx_reg} * {s}) and loads the value into RAX.
Scale factor {s} maps directly to element byte widths in hardware.
<|concept_end|>""")
            blocks.append(f"""<|concept_start|>
Computing array element address with LEA and scale {s}.
<|code|>
; [INTENT] Calculate address of element {idx_reg} with scale {s}
lea rax, [rdi + {idx_reg}*{s}]
<|rule_memory|>
<|commentary|>
Calculates the memory address RDI + ({idx_reg} * {s}) without reading memory.
Writes the calculated effective pointer into RAX in a single ALU cycle.
<|concept_end|>""")

    # 10. Conditional Jumps & Testing
    comparisons = [
        ("cmp rax, rbx\nje .equal_target", "je", "Zero Flag is set (RAX == RBX)"),
        ("cmp rax, rbx\njne .not_equal_target", "jne", "Zero Flag is clear (RAX != RBX)"),
        ("cmp rax, rbx\njg .greater_target", "jg", "signed comparison indicates RAX > RBX"),
        ("cmp rax, rbx\njl .less_target", "jl", "signed comparison indicates RAX < RBX"),
        ("cmp rax, rbx\nja .above_target", "ja", "unsigned comparison indicates RAX > RBX"),
        ("cmp rax, rbx\njb .below_target", "jb", "unsigned comparison indicates RAX < RBX"),
        ("test rax, rax\njz .zero_target", "jz", "RAX contains zero"),
        ("test rax, rax\njnz .nonzero_target", "jnz", "RAX contains non-zero value"),
        ("test rax, 1\njz .even_target", "jz", "least significant bit is zero (even integer)"),
        ("test rax, 1\njnz .odd_target", "jnz", "least significant bit is one (odd integer)"),
    ]
    for code, mnemonic, explanation in comparisons:
        blocks.append(f"""<|concept_start|>
Conditional jump using {mnemonic}.
<|code|>
; [INTENT] Branch if {explanation}
{code}
<|rule_control_flow|>
<|commentary|>
Evaluates the condition flags in EFLAGS. Branches to target if {explanation}.
Otherwise execution falls through to the next sequential instruction.
<|concept_end|>""")

    # 11. Branchless CMOV selections
    cmov_ops = [
        ("cmovz rax, rbx", "cmovz", "Zero Flag is set (condition equal)"),
        ("cmovnz rax, rbx", "cmovnz", "Zero Flag is clear (condition not equal)"),
        ("cmovg rax, rbx", "cmovg", "signed greater than condition"),
        ("cmovl rax, rbx", "cmovl", "signed less than condition"),
        ("cmova rax, rbx", "cmova", "unsigned above condition"),
        ("cmovb rax, rbx", "cmovb", "unsigned below condition"),
    ]
    for code, mnemonic, explanation in cmov_ops:
        blocks.append(f"""<|concept_start|>
Branchless conditional assignment using {mnemonic}.
<|code|>
; [INTENT] Assign rbx to rax if {explanation}
{code}
<|rule_register|>
<|commentary|>
Performs conditional move without branching. If {explanation}, RAX is updated with RBX.
Eliminates branch misprediction pipeline flushes on modern superscalar processors.
<|concept_end|>""")

    # 12. Complete Real Algorithms
    algorithms = [
        ("strlen_64", """strlen_64:
    ; [INTENT] Compute length of null-terminated string rdi
    xor rax, rax
.loop:
    cmp byte [rdi + rax], 0
    je .done
    inc rax
    jmp .loop
.done:
    ret""", "Calculates string length by scanning for the null terminating byte and returns character count in RAX."),
        
        ("memcpy_64", """memcpy_64:
    ; [INTENT] Copy rdx bytes from rsi to rdi
    mov rax, rdi
    xor rcx, rcx
.copy_loop:
    cmp rcx, rdx
    je .done
    mov r8b, [rsi + rcx]
    mov [rdi + rcx], r8b
    inc rcx
    jmp .copy_loop
.done:
    ret""", "Copies RDX bytes from source RSI to destination RDI using temporary byte register R8B, returning destination pointer in RAX."),

        ("memset_64", """memset_64:
    ; [INTENT] Fill rdx bytes at rdi with byte sil
    mov rax, rdi
    xor rcx, rcx
.fill_loop:
    cmp rcx, rdx
    je .done
    mov [rdi + rcx], sil
    inc rcx
    jmp .fill_loop
.done:
    ret""", "Fills RDX bytes of memory at RDI with byte pattern in SIL, returning base pointer in RAX."),

        ("strcmp_64", """strcmp_64:
    ; [INTENT] Compare null-terminated strings rdi and rsi
.cmp_loop:
    mov al, [rdi]
    mov bl, [rsi]
    cmp al, bl
    jne .diff
    test al, al
    jz .equal
    inc rdi
    inc rsi
    jmp .cmp_loop
.diff:
    movzx eax, al
    movzx ebx, bl
    sub eax, ebx
    ret
.equal:
    xor eax, eax
    ret""", "Lexicographical string comparison returning negative, zero, or positive integer in EAX."),

        ("find_max_element", """find_max_element:
    ; [INTENT] Find maximum signed 64-bit integer in array rdi with length rsi
    mov rax, [rdi]
    mov rcx, 1
.max_loop:
    cmp rcx, rsi
    jge .done
    mov rdx, [rdi + rcx*8]
    cmp rdx, rax
    cmovg rax, rdx
    inc rcx
    jmp .max_loop
.done:
    ret""", "Scans array RDI for largest signed element using branchless CMOVG to eliminate branch mispredictions."),

        ("euclidean_gcd", """euclidean_gcd:
    ; [INTENT] Greatest common divisor of rdi and rsi
.gcd_step:
    test rsi, rsi
    jz .done
    xor rdx, rdx
    mov rax, rdi
    div rsi
    mov rdi, rsi
    mov rsi, rdx
    jmp .gcd_step
.done:
    mov rax, rdi
    ret""", "Computes greatest common divisor using Euclidean division remainder loop until divisor reaches zero."),

        ("popcount_fallback", """popcount_fallback:
    ; [INTENT] Count set bits in rdi without hardware popcnt
    xor rax, rax
.count_loop:
    test rdi, rdi
    jz .done
    mov rbx, rdi
    dec rbx
    and rdi, rbx
    inc rax
    jmp .count_loop
.done:
    ret""", "Brian Kernighan bit-counting algorithm: clearing lowest set bit via rdi AND (rdi - 1) until zero."),

        ("fast_inv_sqrt", """fast_inv_sqrt:
    ; [INTENT] Fast inverse square root of float in xmm0
    movss xmm1, dword [rel magic_num]
    movd eax, xmm0
    sar eax, 1
    sub eax, 0x5f3759df
    neg eax
    movd xmm0, eax
    ret""", "Quake III fast reciprocal square root bit-cast hack for rapid initial vector normalization approximation."),

        ("simd_vector_dot_product", """simd_vector_dot_product:
    ; [INTENT] Compute dot product of two 4-float vectors in xmm0 and xmm1
    mulps xmm0, xmm1
    movshdup xmm1, xmm0
    addps xmm0, xmm1
    movhlps xmm1, xmm0
    addss xmm0, xmm1
    ret""", "Multiplies four vector elements in parallel with MULPS and accumulates components into scalar float in XMM0."),

        ("branchless_abs_64", """branchless_abs_64:
    ; [INTENT] Compute absolute value of signed integer in rdi
    mov rax, rdi
    cqo
    xor rax, rdx
    sub rax, rdx
    ret""", "Branchless absolute value: arithmetic sign extension into RDX followed by XOR and subtraction."),

        ("power_of_two_check", """power_of_two_check:
    ; [INTENT] Return 1 if rdi is power of two, else 0
    test rdi, rdi
    jle .not_power
    mov rax, rdi
    dec rax
    test rdi, rax
    jnz .not_power
    mov eax, 1
    ret
.not_power:
    xor eax, eax
    ret""", "Checks if integer in RDI is positive and has exactly one bit set using rdi AND (rdi - 1)."),

        ("fnv1a_hash_64", """fnv1a_hash_64:
    ; [INTENT] FNV-1a 64-bit hash of buffer rdi with length rsi
    mov rax, 0xcbf29ce484222325
    mov r8,  0x100000001b3
    xor rcx, rcx
.hash_loop:
    cmp rcx, rsi
    je .done
    movzx rdx, byte [rdi + rcx]
    xor rax, rdx
    imul rax, r8
    inc rcx
    jmp .hash_loop
.done:
    ret""", "64-bit Fowler-Noll-Vo hash FNV-1a multiplying by prime constant 0x100000001B3 after XORing each input byte."),

        ("binary_search_64", """binary_search_64:
    ; [INTENT] Binary search sorted array rdi between rsi and rdx for target rcx
.search_loop:
    cmp rsi, rdx
    jg .not_found
    mov rax, rsi
    add rax, rdx
    shr rax, 1
    mov r8, [rdi + rax*8]
    cmp r8, rcx
    je .found
    jl .search_right
    mov rdx, rax
    dec rdx
    jmp .search_loop
.search_right:
    mov rsi, rax
    inc rsi
    jmp .search_loop
.found:
    ret
.not_found:
    mov rax, -1
    ret""", "Logarithmic binary search over 64-bit sorted integer array returning index in RAX or -1 if absent."),

        ("matrix_vector_mult_4x4", """matrix_vector_mult_4x4:
    ; [INTENT] Multiply 4x4 float matrix rdi by 4D float vector rsi
    movups xmm4, [rsi]
    movups xmm0, [rdi]
    mulps xmm0, xmm4
    movups xmm1, [rdi + 16]
    mulps xmm1, xmm4
    movups xmm2, [rdi + 32]
    mulps xmm2, xmm4
    movups xmm3, [rdi + 48]
    mulps xmm3, xmm4
    haddps xmm0, xmm1
    haddps xmm2, xmm3
    haddps xmm0, xmm2
    movups [rdx], xmm0
    ret""", "Multiplies 4x4 matrix by 4-element column vector using SSE horizontal additions and stores result at RDX."),

        ("string_to_int_atoi", """string_to_int_atoi:
    ; [INTENT] Convert ASCII decimal string rdi to 64-bit integer
    xor rax, rax
.parse_digit:
    movzx rcx, byte [rdi]
    cmp cl, '0'
    jb .parse_done
    cmp cl, '9'
    ja .parse_done
    sub cl, '0'
    imul rax, 10
    add rax, rcx
    inc rdi
    jmp .parse_digit
.parse_done:
    ret""", "Parses ASCII numeric digits into 64-bit scalar integer accumulator RAX until non-digit encountered."),

        ("int_to_hex_string", """int_to_hex_string:
    ; [INTENT] Convert 64-bit integer rdi to 16-character hex string at rsi
    mov rcx, 16
.hex_loop:
    dec rcx
    mov rax, rdi
    and rax, 0x0F
    cmp rax, 10
    sbb al, 0x69
    das
    mov [rsi + rcx], al
    shr rdi, 4
    test rcx, rcx
    jnz .hex_loop
    ret""", "Formats 64-bit integer into hexadecimal ASCII characters using branchless nibble arithmetic."),
    ]

    for name, code, commentary in algorithms:
        blocks.append(f"""<|concept_start|>
Implementation of {name} in x86_64 NASM.
<|code|>
{code}
<|rule_control_flow|>
<|commentary|>
{commentary}
<|concept_end|>""")

    # 13. ABI, Calling Conventions, and Stack Rules
    abi_blocks = [
        ("System V AMD64 function argument passing", """mov rdi, 10
mov rsi, 20
mov rdx, 30
mov rcx, 40
mov r8,  50
mov r9,  60
call process_six_args""", "<|rule_control_flow|>", "First six integer arguments passed in registers RDI, RSI, RDX, RCX, R8, R9 under System V ABI."),

        ("Microsoft x64 shadow store reservation", """sub rsp, 40
call win64_subroutine
add rsp, 40""", "<|rule_memory|>", "Caller allocates 32 bytes shadow space plus 8 bytes stack alignment before calling Win64 subroutines."),

        ("System V callee-saved register preservation", """push rbx
push r12
push r13
; function body
pop r13
pop r12
pop rbx
ret""", "<|rule_memory|>", "Callee must preserve RBX, R12, R13, R14, R15, RBP, RSP across function calls under System V ABI."),

        ("Leaf function omitting frame pointer", """mov rax, rdi
add rax, rsi
ret""", "<|rule_register|>", "Leaf functions that do not allocate stack space or call other functions omit RBP frame setup for maximum speed."),

        ("16-byte stack alignment before CALL", """push rbp
mov rbp, rsp
sub rsp, 32
call external_function
mov rsp, rbp
pop rbp
ret""", "<|rule_memory|>", "Both System V and Win64 ABIs require RSP to be 16-byte aligned at the point of CALL instruction."),

        ("Scalar integer return value in RAX", """mov eax, 42
ret""", "<|rule_register|>", "All 32-bit and 64-bit integer return values are transmitted in RAX (or EAX for 32-bit types)."),

        ("Scalar floating-point return value in XMM0", """movss xmm0, [rel result_float]
ret""", "<|rule_register|>", "Floating-point scalar return values are transmitted in the lowest lane of XMM0."),

        ("Linux x86_64 sys_exit system call", """mov rax, 60
xor rdi, rdi
syscall""", "<|rule_control_flow|>", "Invokes Linux sys_exit (syscall 60 in RAX) with exit status code 0 in RDI."),

        ("Linux x86_64 sys_write system call", """mov rax, 1
mov rdi, 1
mov rsi, buffer
mov rdx, 64
syscall""", "<|rule_control_flow|>", "Invokes Linux sys_write (syscall 1 in RAX) writing 64 bytes from buffer RSI to stdout file descriptor 1 in RDI."),
    ]
    for title, code, rule, commentary in abi_blocks:
        blocks.append(f"""<|concept_start|>
{title}.
<|code|>
{code}
{rule}
<|commentary|>
{commentary}
<|concept_end|>""")

    # 14. AVX & SIMD vector rules
    simd_blocks = [
        ("Fused Multiply-Add AVX vector instruction", "vfmadd231ps ymm0, ymm1, ymm2", "<|rule_register|>", "Computes ymm0 = (ymm1 * ymm2) + ymm0 for eight 32-bit single-precision floats across 256-bit YMM register."),
        ("Aligned 128-bit vector load", "movaps xmm0, [rdi]", "<|rule_memory|>", "Loads 128-bit vector from memory address RDI aligned to 16 bytes into XMM0."),
        ("Unaligned 128-bit vector load", "movups xmm0, [rdi]", "<|rule_memory|>", "Loads 128-bit vector from arbitrary memory address RDI into XMM0."),
        ("Parallel 4-float vector addition", "addps xmm0, xmm1", "<|rule_register|>", "Adds four 32-bit floats in XMM1 to XMM0 in parallel."),
        ("Parallel 4-float vector multiplication", "mulps xmm0, xmm1", "<|rule_register|>", "Multiplies four 32-bit floats in XMM0 by XMM1 in parallel."),
        ("Scalar single float square root", "sqrtss xmm0, xmm1", "<|rule_register|>", "Computes square root of lowest float in XMM1 into XMM0."),
        ("Fast reciprocal square root approximation", "rsqrtss xmm0, xmm1", "<|rule_register|>", "Approximate 1 / sqrt(x) in under 4 clock cycles using SSE hardware."),
    ]
    for title, code, rule, commentary in simd_blocks:
        blocks.append(f"""<|concept_start|>
{title}.
<|code|>
{code}
{rule}
<|commentary|>
{commentary}
<|concept_end|>""")

    content = "\n\n".join(blocks)
    target_path = os.path.join("core", "asset_sample", "assembly_training_corpus.txt")
    with open(target_path, "w", encoding="utf-8") as f:
        f.write(content)
        f.write("\n")
    print(f"Generated {len(blocks)} blocks into {target_path} ({len(content)} bytes).")

if __name__ == "__main__":
    generate_corpus()
