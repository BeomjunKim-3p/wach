# WACH 기여 가이드
이 프로젝트에 관심을 가져주시고 기여해주셔서 감사합니다.

[English](CONTRIBUTING.MD) | 한국어(Korean)

## 기여 방법

이 프로젝트에 여러 방법으로 기여하실 수 있습니다.

- **Bug report**: 문제가 있으면 재현 단계와 함께 이슈를 열어주세요
- **문서 개선**: 오타 수정, 설명 보완, 예시 추가, 오류 정정 등을 통해 기여하실 수 있습니다.
- **코드 작성**: 버그픽스, 기능 추가, 성능 향상, 구현등을 통해 기여하실 수 있습니다.

## 시작하기

### 전제조건

- [Git](https://git-scm.com/)
- [SystemC](https://systemc.org/)
- C++23 컴파일러

### 준비하기
```bash
# 1. 깃허브에서 저장소를 포크합니다

# 2. 포크된 저장소를 받아옵니다.
git clone https://github.com/<your-username>/wach.git
cd wach

# 3. 주 저장소에서 변경사항을 가져오기 위해 업스트림 원격을 추가합니다
git remote add upstream https://github.com/BeomjunKim-3p/wach.git

# (Optional). SystemC 설계를 위한 SystemC를 설치합니다(ubuntu 24.04기준).
sudo apt update
sudo apt install build-essential cmake
   ### 의존성 디렉토리가 없는경우
   mkdir dependencies
cd dependencies
wget https://github.com/accellera-official/systemc/archive/refs/tags/3.0.2.tar.gz
tar -xvzf 3.0.2.tar.gz
cd systemc-3.0.2
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=./build/install -DCMAKE_CXX_STANDARD=23 -DCMAKE_CXX_STANDARD_REQUIRED=ON
cd build
make -j$(nproc)
make install
# SystemC 경로설정
export SYSTEMC_HOME=<당신의 프로젝트 경로>/dependencies/systemc-3.0.2/build/install
export LD_LIBRARY_PATH=$SYSTEMC_HOME/lib:$LD_LIBRARY_PATH
```

이제 기여를 시작할 준비가 끝났습니다.

## 개발 워크플로우
gitflow를 사용합니다.

### 브랜치

'develop'브랜치에서 기능 브랜치를 만듭니다.

```bash
git checkout develop
git pull upstream develop
git checkout -b feature/your-feature-name
```
브랜치 이름 앞에 브랜치의 역할을 설명하는 접두사를 붙입니다.
| 접두사 | 목적 |
|:---|:---|
| `feature/` | 새 기능 |
| `fix/` | 버그픽스 |
| `docs/` | 문서 변경만 포함 |
| `refactor/` | 동작 변화 없는 코드 리펙토링 |
| `test/` | 테스트 추가 또는 수정 |
| `chore/` | 빌드 업무 수정, 패키지 매니저 수정 |

- develop브랜치 (설계 및 기능구현 중심)
  - 주요 작업: 아키텍쳐 변경 및 모듈 추가
  - feature 브랜치: 단계별로 생성하기보다 모듈 단위로 생성합니다.
    - 예) ```feature/alu/docs``` 설계 후 develop에 병합

    - 예) ```feature/alu/systemc``` 구현 -> 검증 후 develop에 병합
    - 예) ```feature/alu/rtl```구현 -> 검증 후 develop에 병합
  - release 브랜치: 저수준 단계 검증 및 튜
    - 예) ```feature/alu/spice```구현 -> 검증 후 develop에 병합
  - main 브랜치: 최종 검증 완료
    - 기능 검증. 타이밍 검증, 전기적 검정 종료 후 제작 가능하고 문제가 발생하지 않는 상태일떄, main브랜치에 병합하고 버전을 올린다

### 커밋 메시지

[Conventional Commits](https://www.conventionalcommits.org/) 형식을 따릅니다.

### 코드 품질
변경사항을 제출하기 전 모든 빌드와 테스트가 통과하는지 확인하세요.

### 프로젝트 구조
```
/
├─ 아키텍쳐1 이름/          # 각 아키텍쳐의 시작입니다.
│  ├─ 아키텍쳐1 이름{모듈명 (예: i/m/a/f/d/c/b/v)}/    # 아키텍쳐 내부 모듈입니다.
│  │   ├─ docs/             # 아키텍쳐의 isa 설계, cpu내부 구조 설계 등 설계문서와 기타 문서가 들어갑니다.
│  │   ├─ sim_systemc/      # SystemC를 이용해 설계한 아키텍쳐를 검증하는 곳입니다.
│  │   ├─ rtl_Sverilog/     # SystemVerilog를 이용해 합성 가능한 RTL을 설계합니다.
│  │   └─ sim_spice/        # 합성 된 로직게이트들을 실제로 배치해보고 제대로 작동하는지 검증합니다.
│  └─ 아키텍쳐1 이름{모듈명 (예: i/m/a/f/d/c/b/v)}/  
└─ 아키텍쳐2 이름/
```

## 변경 제출

### 이슈 열기

큰 변경을 시작하기 전에는 먼저 이슈를 열어 접근 방식을 논의해 주세요. 이렇게 하면 중복 작업을 줄이고 기여 방향이 프로젝트와 맞는지 확인할 수 있습니다.

버그를 보고할 때는 다음 정보를 포함해 주세요.

1. WACH 버전과 모듈
2. 재현 단계
3. 기대 동작과 실제 동작
4. 관련 로그 또는 오류 메시지

### Pull Request 절차

1. **PR을 집중되게 유지**: 하나의 PR에는 하나의 논리적 변경만 포함하세요. 서로 독립적인 변경이 여러 개라면 별도 PR로 제출하세요.
2. **Test 작성**: 동작 변경에는 test를 추가하거나 update하세요.
3. **문서 update**: 사용자에게 보이는 동작이 바뀌면 관련 문서를 변경하세요.
4. **PR 작성**: 변경 내용과 이유를 설명하세요.

### PR 제목 규칙

commit message와 같은 [Conventional Commits](https://www.conventionalcommits.org/) 형식을 사용합니다.


### 하향식 검증
이 프로젝트에서는 하향식으로 설계하고 개발합니다. 

항상 하위 레이어와 상위 레이어의 동작이 동일한지 증명할수 있는 자료를 첨부해 PR을 제출해야 합니다.

