LSB 0 기준

# R-Type
UNUSED(4) FUNCT(4) RS1(4) RS2(4) OPCODE(4) RD(4) 

|operand|value|
|--|--|
|OPCODE| 0x0 |
|RD|계산결과 저장할 레지스터|
|RS1|소스 1|
|RS2|소스 2|

|FUNCT|세부기능|
|----|-----|
|0x0|ADD|
|0x1|SUB|
|0x2|AND|
|0x3|OR|
|0x4|SLL|
|0x5|SLT|
|0x6|SLTU|
|0x7|SRL|
|0x8|SRA|
나머지는 예약용

# I-Type (상수계산, 간접점프)
IMMIDIATE\[7:4\](4) FUNCT(4) RS(4) IMMIDIATE\[3:0\](4) OPCODE(4) RD(4)

|operand|value|
|--|--|
|OPCODE|0x1|
|RD|계산결과 저장할 레지스터|
|RS|소스 1|
|IMMIDIATE|소스 2 즉시값|
FUNCT는 R-Type에
|FUNCT|세부기능|
|---|----|
|0xF|JALR|
추가

JALR의 경우:
|operand|value|
|--|--|
|RD|반환 주소값 저장할 레지스터|
|RS|점프할 포인터 주소|
|IMMIDIATE(8)| 점프 주소 포인터 오프셋|

# I-Type(메모리로드)
IMMIDIATE\[11:4\](8) RS(4) IMMIDIATE\[3:0\](4) OPCODE(4) RD(4)
|operand|value|
|--|--|
|OPCODE|0x2|
|RD|메모리 값 저장할 레지스터|
|RS|메모리 값 읽어올 포인터|
|IMMIDIATE|읽기 대상 포인터 오프셋(바이트기준; Signed)|

# S-Type
IMMIDIATE\[11:4\](8) RS1(4) IMMIDIATE\[3:0\](4) OPCODE(4) RS2(4) 
|operand|value|
|--|--|
|OPCODE|0x3|
|RS2|메모리에 저장할 데이터 담긴 레지스터|
|RS1|메모리 저장할 주소 포인터|
|IMMIDIATE|쓰기 대상 포인터 오프셋(바이트기준; Signed)|

# B-Type
IMMIDIATE\[7:4\](4) FUNCT(4) RS1(4) IMMIDIATE\[3:0\](4) OPCODE(4) RS2(4)
|operand|value|
|--|--|
|OPCODE|0x04|
|RS2|비교할 두번째 수|
|RS1|비교할 첫번쨰 수|
|IMMIDIATE|조건 만족시 이동할 오프셋(현재위치기준; 바이트기준; Signed)|

|FUNC|세부기능|설명|
|----|------|------|
|0x0|BEQ|Branch if Equal|
|0x1|BNE|Branch Not Equal|
|0x2|BLT|Branch Less Than (Signed)|
|0x3|BLTU|Branch Less Than Unsigned|
BGT, BGTL은 
``` ASM
B-Type RS2 RS1 3 BLT(U)
J-Type foo bar baz
  ```
으로 대체 가능하기 떄문에 생략

# J-Type(절대주소)
IMMIDIATE\[7:0\](8) IMMIDIATE\[15:8\](8) OPCODE(4) RD(4)
|operand|value|
|--|--|
|OPCODE|0x5|
|RD|반환 주소값 저장할 레지스터|
|IMMIDIATE|점프할 주소(절대주소)|

# J-Type(상대주소)
IMMIDIATE\[7:0\](8) IMMIDIATE\[15:8\](8) OPCODE(4) RD(4)
|operand|value|
|--|--|
|OPCODE|0x6|
다른 operand J-Type(절대주소)와 동일.

IMMIDIATE의 경우 절대주소 대신 현재 PC에 적용할 Signed 오프셋 값
PIC 위해 만듬

# SYSTEM
INSTRUCTION_SPECIAL_RESERVED2\[7:4\](4) FUNCT(4) INSTRUCTION_SPECIAL_RESERVED1(4) INSTRUCTION_SPECIAL_RESERVED2\[3:0\](4) OPCODE(4) INSTRUCTION_SPECIAL_RESERVED0(4)
|operand|value|
|--|--|
|OPCODE|0x7|

## ECALL EBRAKE
|FUNCT|세부사항| 
|--|--|
|0x0|ECALL|
|0x1|EBRAKE|


INSTRUCTION_SPECIAL_RESERVED:
|Number|detail|
|--|--|
|0|UNUSED|
|1|UNUSED|
|2|UNUSED|

## CSRR
|operand|value|
|--|--|
|FUNCT|0x2|
|RD|값 읽어올 레지스터|
|CSR_ADDR|대상 모델특수레지스터 주소|

INSTRUCTION_SPECIAL_RESERVED:
|Number|detail|
|--|--|
|0|RD|
|1|UNUSED|
|2|CSR_ADDR|

## CSRW
|operand|value|
|--|--|
|FUNCT|0x3|
|RS|쓸 값 있는 레지스터|
|CSR_ADDR|대상 모델특수레지스터 주소|

INSTRUCTION_SPECIAL_RESERVED:
|Number|detail|
|--|--|
|0|UNUSED|
|1|RS|
|2|CSR_ADDR|
## CSRS
|operand|value|
|--|--|
|FUNCT|0x4|
|RD|쓸 값 있는 레지스터|
|RS|비트마스크|
|CSR_ADDR|대상 모델특수레지스터 주소|

INSTRUCTION_SPECIAL_RESERVED:
|Number|detail|
|--|--|
|0|RD|
|1|RS|
|2|CSR_ADDR|
RS중 1로 되어있는 비트만 RD에 있는 값으로 CSR 레지스터 값 바꿈
