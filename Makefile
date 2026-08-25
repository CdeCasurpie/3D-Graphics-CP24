.PHONY: all HelloTriangle ChaosAlgorithm RecursiveAlgorithm SphereCHE Transformaciones Tetrahedron clean run-HelloTriangle run-ChaosAlgorithm run-RecursiveAlgorithm run-SphereCHE run-Transformaciones run-Tetrahedron

all:
	@echo "Configuring CMake..."
	@cmake -B build
	@echo "Building all targets..."
	@cmake --build build

HelloTriangle:
	@cmake -B build
	@cmake --build build --target HelloTriangle

ChaosAlgorithm:
	@cmake -B build
	@cmake --build build --target ChaosAlgorithm

RecursiveAlgorithm:
	@cmake -B build
	@cmake --build build --target RecursiveAlgorithm

SphereCHE:
	@cmake -B build
	@cmake --build build --target SphereCHE

Transformaciones:
	@cmake -B build
	@cmake --build build --target Transformaciones

Tetrahedron:
	@cmake -B build
	@cmake --build build --target Tetrahedron

run-HelloTriangle: HelloTriangle
	@./build/HelloTriangle

run-ChaosAlgorithm: ChaosAlgorithm
	@./build/ChaosAlgorithm

run-RecursiveAlgorithm: RecursiveAlgorithm
	@./build/RecursiveAlgorithm

run-SphereCHE: SphereCHE
	@./build/SphereCHE

run-Transformaciones: Transformaciones
	@./build/Transformaciones

run-Tetrahedron: Tetrahedron
	@./build/Tetrahedron

clean:
	@echo "Cleaning build directory..."
	@rm -rf build
