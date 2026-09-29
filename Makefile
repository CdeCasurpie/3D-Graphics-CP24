.PHONY: all HelloTriangle ChaosAlgorithm RecursiveAlgorithm SphereCHE Transformaciones Camara Simplificacion Tetrahedron ArcballCamera Iluminacion ShadowMapping TexturesAndMaps ModelLoading clean run-HelloTriangle run-ChaosAlgorithm run-RecursiveAlgorithm run-SphereCHE run-Transformaciones run-Camara run-Simplificacion run-Tetrahedron run-ArcballCamera run-Iluminacion run-ShadowMapping run-TexturesAndMaps run-ModelLoading

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

Camara:
	@cmake -B build
	@cmake --build build --target Camara

Simplificacion:
	@cmake -B build
	@cmake --build build --target Simplificacion

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

run-Camara: Camara
	@./build/Camara

run-Simplificacion: Simplificacion
	@./build/Simplificacion

run-Tetrahedron: Tetrahedron
	@./build/Tetrahedron

clean:
	@echo "Cleaning build directory..."
	@rm -rf build

ArcballCamera:
	@cmake -B build
	@cmake --build build --target ArcballCamera

run-ArcballCamera: ArcballCamera
	@./build/ArcballCamera

Iluminacion:
	@cmake -B build
	@cmake --build build --target Iluminacion

run-Iluminacion: Iluminacion
	@./build/Iluminacion

run-FastMarching:
	cmake -B build && cmake --build build --target FastMarching && ./build/FastMarching

ShadowMapping:
	@cmake -B build
	@cmake --build build --target ShadowMapping

run-ShadowMapping: ShadowMapping
	@./build/ShadowMapping

TexturesAndMaps:
	@cmake -B build -DCMAKE_BUILD_TYPE=Release
	@cmake --build build --target TexturesAndMaps

run-TexturesAndMaps: TexturesAndMaps
	@./build/TexturesAndMaps

ModelLoading:
	@cmake -B build -DCMAKE_BUILD_TYPE=Release
	@cmake --build build --target ModelLoading

run-ModelLoading: ModelLoading
	@./build/ModelLoading


Project1:
	@cmake -B build -DCMAKE_BUILD_TYPE=Release
	@cmake --build build --target Project1

run-Project1: Project1
	@./build/Project1
