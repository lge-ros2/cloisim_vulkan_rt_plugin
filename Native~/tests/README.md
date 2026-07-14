# Native Tests

CTest 기반의 외부 의존성 없는 기본 검증입니다.

- `cloisim_rt_abi_layout_test`: C/C# 공유 capability ABI의 크기와 필드 오프셋
- `cloisim_rt_shader_loader_test`: 정상/손상/누락 SPIR-V 처리
- `cloisim_rt_instance_layout_test`: TLAS instance 구조와 필드 round-trip
- `cloisim_rt_spirv_outputs_test`: CMake가 생성한 셰이더 파일과 SPIR-V magic

GPU/Unity 런타임 검증은 별도의 PlayMode smoke test로 추가해야 합니다.
