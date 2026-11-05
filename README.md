# SpriteUI

**Versão:** 0.5.1 — núcleo C++20 (layout, input, widgets, docking, texto, config), baseado na especificação V4.2. Revisão e conformidade: `docs/REVIEW_V4_2.md`. Convenções: `docs/CODING_STANDARDS.md`.

SpriteUI é o início executável de uma biblioteca de interface 2D orientada a dados. O núcleo matemático, input, widgets lógicos, animação e extração de comandos são independentes de ECS e de API gráfica, para que Flecs e Diligent possam ser integrados por adaptadores sem contaminar os contratos públicos.

> Importante: esta entrega não é ainda uma GUI com janela visível. O backend incluído é `NullRenderBackend`, usado para validar pacotes e o fluxo CPU. O backend Diligent, a integração Flecs, a renderização GPU e a tipografia ainda são etapas futuras.

## Funcionalidades implementadas

- ABI `SpriteInstanceGPU` com 80 bytes, alinhamento 16 e verificações de layout/offset em compilação.
- Conversão de cor sRGB → linear e premultiplicação de alpha.
- Canvas Screen/World e modo Unlit/ClusterLit com validação das combinações permitidas.
- Anchors/offsets e transformação 2D com pivô, escala e rotação.
- Transformação local em pixels para hit-testing coerente com o tamanho lógico do widget.
- Handles com índice + geração para evitar referências recicladas.
- Roteamento de input com hover, foco, captura, click, wheel, move e cancelamento por perda de foco.
- Propagação de eventos pelos ancestrais (`HitTarget::parent`) e consumo de input por alvo/ancestral.
- Navegação espacial por teclado/gamepad e eventos semânticos `Activate`/`Cancel`.
- Slider com clamp, domínio, passo opcional e proteção contra largura zero.
- ScrollView com limites `contentSize - viewportSize`.
- ToonMotion básico com easing, `Override`/`Queue`/`Ignore`, pausa/cancelamento e shake sem drift.
- Flags de invalidação por subsistema.
- Extração de instâncias, ordenação visual e batching apenas entre comandos consecutivos compatíveis.
- Árvore de layout com relações explícitas, remoção recursiva segura e validação de parent handles.
- Alinhamento transversal de itens (Stretch/Start/Center/End).
- Interseção de clipping hierárquico para renderização e clip bounds em hit-testing.
- Validação estrutural de `FramePacket` antes da submissão.
- Shader HLSL de referência alinhado à ABI.
- `NullRenderBackend`, exemplo CLI e testes automatizados.

## Ainda não implementado

| Módulo | Estado |
|---|---|
| Flexbox: wrap, align-self distinto de align-items, baseline | Pendente (grow/shrink com freeze de min/max e justify-content implementados) |
| Flecs ECS adapter | Pendente |
| Diligent Engine backend, buffers reais e fences | Pendente |
| SDL3/GLFW, janela, clipboard, IME e input multi-pointer | Pendente |
| Widgets visuais (desenho), drag & drop de docking | Pendente (lógica de janelas, abas e dock tree implementada) |
| HarfBuzz, FreeType, grapheme clusters, MSDF atlas | Pendente (layout geométrico de glifos já shaped implementado; `TextWrap::Word` ainda quebra por glifo) |
| ToonMotion timeline/keyframes e integração ECS | Pendente |
| KTX2/Basis, streaming e eviction por fences | Pendente |
| Light Prism ClusterLit, Bloom, Acrylic Blur e sombras | Pendente |
| Partículas e nodes de editor | Pendente |
| QuickJS sandbox e FFI | Pendente |

O estado completo e a correspondência com o documento V4.2 estão em `docs/IMPLEMENTATION_STATUS.md` e `docs/V4_2_IMPLEMENTATION_MAP.md`.

## Requisitos

- CMake 3.21+
- Compilador C++20 (MSVC 2022, GCC 12+ ou Clang 15+ recomendado)
- Ninja opcional (recomendado)
- DXC opcional para compilar o shader de referência

O núcleo CPU não requer dependências externas.

## Compilar e testar

Windows: execute `scripts\build.bat` (usa Ninja se disponível; caso contrário, usa o gerador padrão do CMake).

Linux/macOS:

```sh
chmod +x scripts/build.sh
./scripts/build.sh
```

Manual:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Build Release:

```sh
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --parallel
ctest --test-dir build-release --output-on-failure
```

## Executar o exemplo

```sh
./build/spriteui_hello
```

No Windows: `build\spriteui_hello.exe` ou `build\spriteui_interaction_demo.exe`.

O exemplo `spriteui_hello` monta três elementos, extrai instâncias, cria batches e imprime estatísticas. O exemplo `spriteui_interaction_demo` demonstra hit-testing com rotação, foco/captura, bubbling, navegação semântica, slider e scroll. Nenhum dos dois abre janela nem envia comandos para uma GPU real.

## Exemplo de hit target transformado

`BuildHitTestTransform` produz a transformação de coordenadas locais em pixels para coordenadas lógicas de ecrã. `localSize` deve conter o tamanho em pixels lógicos:

```cpp
UILayoutRect layout{{120.0f, 80.0f}, {240.0f, 64.0f}, true, false};
UIRectTransform transform;
transform.pivot = {0.5f, 0.5f};
transform.rotationRadians = 0.15f;
transform.scale = {1.0f, 1.0f};

auto localToScreen = BuildHitTestTransform(layout, transform);
if (localToScreen) {
    Input::HitTarget target;
    target.handle = {42, 1};
    target.localToScreen = *localToScreen;
    target.localSize = layout.resolvedSize;
    target.focusable = true;
}
```

## Integração pretendida com uma engine

1. O ECS (por exemplo, Flecs) mantém componentes e entidades persistentes.
2. Um adaptador gera `UIRenderItem` e `HitTarget` a partir dos componentes alterados.
3. O núcleo SpriteUI processa input, navegação, animação e extração.
4. `RenderExtractor` cria um `FramePacket` ordenado; o validador garante que batches cobrem as instâncias sem lacunas/overlaps.
5. Um backend Diligent futuro consome o `FramePacket`, mantém buffers/ring e não recicla memória GPU antes das fences.
6. Light Prism é consultada apenas para Canvas `World` com `ClusterLit`; HUD Screen é `Unlit` por padrão.

Não armazene ponteiros da RHI nos componentes ECS. Utilize handles com geração e um registry no backend.

## Contrato de alpha

- Cor de albedo/textura deve ser amostrada em espaço linear (ou formato SRGB com decodificação correta).
- `SpriteInstanceGPU::tint` é RGBA linear pré-multiplicado.
- O blending Source-Over esperado usa `ONE / INV_SRC_ALPHA` para cor. O backend deve definir e validar o estado completo de blending para a API escolhida.
- Texturas de dados (normal, máscara, roughness) não devem sofrer decodificação sRGB.

## ABI `SpriteInstanceGPU` — 80 bytes

| Offset | Campo | Bytes |
|---:|---|---:|
| 0 | `transformRow0` | 16 |
| 16 | `transformRow1` | 16 |
| 32 | `uvRect` | 16 |
| 48 | `tint` | 16 |
| 64 | `resourceIds` | 16 |
| | **Total** | **80** |

## Licença

MIT. Dependências externas a integrar no futuro mantêm as respetivas licenças.

## Layout Core (incremental)

`SpriteUi/Layout/Layout.hpp` fornece `LayoutEngine` com fases `Measure` e `Arrange`, direção Row/Column, tamanho Fixed/Percent/Auto/Flex, grow/shrink, gap, padding, margins e limites min/max. `LayoutTree` permite compor uma árvore com `UIHandle` sem dependência de ECS.

```cpp
using namespace ToolEngine::SpriteUi;
LayoutTree tree;
FlexItem rootStyle; rootStyle.direction = LayoutDirection::Column;
rootStyle.width = {SizeMode::Fixed, 640};
rootStyle.height = {SizeMode::Fixed, 480};
auto root = tree.Create({}, rootStyle);
FlexItem child; child.width = {SizeMode::Flex, 0}; child.height = {SizeMode::Fixed, 40};
child.grow = 1.0f;
tree.Create(root, child);
tree.Layout({{0,0},{640,480}});
```

Este é um subset de Flex: o algoritmo atual não implementa ainda o ciclo completo de freeze/re-distribuição para min/max, wrap, justify-content, align-self ou percentuais com bases intrínsecas complexas. `BuildClipHierarchy` recebe clips em ordem pai-antes-do-filho e retorna interseções efetivas; o índice resultante pode ser atribuído a `UIRenderItem::clipIndex`. Para input, `HitTarget::clipRectLogical` deve receber o clip efetivo em coordenadas lógicas. Veja `docs/IMPLEMENTATION_STATUS.md` para limites e próximos passos.
