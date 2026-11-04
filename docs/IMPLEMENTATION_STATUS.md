# Estado da implementação — SpriteUI 0.3.0 (Layout Tree + Clip Hierarchy)

Base normativa: documento SpriteUi V4.2 fornecido pelo utilizador. Esta implementação prioriza um núcleo C++20 compilável e testável antes de acoplar ECS, RHI ou bibliotecas externas.

## Implementado nesta etapa

- ABI `SpriteInstanceGPU` de 80 bytes, alinhamento 16 e verificações de offsets/tipos.
- `UIMaterialGPU` base e configuração de Canvas Screen/World com validação do modo de iluminação.
- Matemática 2D, anchors/offsets, transformação de quad e transformação local em pixels para hit-testing com pivô, rotação e escala.
- Handles lógicos com índice + geração.
- Input snapshot, hit-testing inverso por paint order, foco, captura, hover, click, wheel, cancelamento por perda de foco da janela e proteção contra `frameId` repetido.
- Propagação de eventos aos ancestrais por `HitTarget::parent`; o consumo de input considera bloqueio no alvo ou num ancestral. O campo `blocksInput` consome input do mundo, mas não interrompe a propagação dentro da árvore UI.
- Navegação espacial e roteador semântico para `Activate`/`Cancel`.
- Slider com validação de domínio, passo opcional e proteção contra largura zero.
- ScrollView com clamp por `contentSize - viewportSize`.
- ToonMotion básico com easing, políticas `Override`/`Queue`/`Ignore`, pausa/cancelamento e shake não cumulativo.
- Flags de invalidação por subsistema.
- Render extraction com ordenação visual estável, batches apenas consecutivos, clip rects e ABI de 80 bytes.
- LayoutTree com filhos explícitos, validação de parent handle e remoção recursiva de subárvores.
- Alinhamento transversal Stretch/Start/Center/End no layout subset.
- `BuildClipHierarchy` com interseção de clips pai/filho; hit-testing respeita clip lógico efetivo.
- Validador de `FramePacket`; `NullRenderBackend` rejeita pacotes incoerentes.
- Shader HLSL de referência e exemplo executável sem GPU.
- CMake com targets de instalação e `find_package(SpriteUI CONFIG)`.

## Pendente — não declarado como implementado

| Módulo | Estado |
|---|---|
| Flecs ECS adapter | Pendente |
| Diligent Engine backend e gestão real de buffers/fences | Pendente |
| Compilação/validação de HLSL por DXC | Não validado neste ambiente (DXC ausente) |
| Flexbox completo (wrap, justify, freeze/re-distribuição min/max, percentuais intrínsecos) | Parcial: Measure/Arrange, grow/shrink simplificado e alinhamento transversal |
| Canvas/Scene graph e clipping hierárquico completo/automático | Parcial: árvore de layout + utilitário de interseção; extração/transformação automática pendente |
| Backend de janela/OS, SDL3/GLFW, multi-pointer e IME | Pendente |
| HarfBuzz, FreeType, segmentação Grapheme, MSDF atlas e edição Unicode | Pendente |
| Widgets visuais completos (button, dropdown, text input, tabs, docking) | Pendente; estado lógico de slider/scroll e input base já existe |
| ToonMotion timeline/keyframes, múltiplas tracks persistentes em ECS e integração com Flecs | Pendente |
| Persistência JSON com schema/migrações e escrita atómica | Pendente |
| Registry real de texturas, KTX2/Basis, streaming e eviction por fences | Pendente |
| Light Prism ClusterLit, normal mapping, Bloom e Acrylic Blur | Pendente |
| Partículas 2D, gráficos e nodes de editor | Pendente |
| QuickJS sandbox e FFI | Pendente |
| Profiling em GPU real e benchmark de escala | Pendente |

## Limitações conhecidas do MVP

- `UIEventRouter::Process` devolve um `std::vector` de eventos por chamada; em produção deve ser adaptado para buffer reutilizável/PMR ou arena por frame se o perfil justificar.
- O hit-test é linear sobre os `HitTarget` fornecidos. Índices espaciais, grids ou árvores só devem ser adicionados após benchmark com workloads reais.
- A árvore de pais deve ser acíclica e os `paintOrder` devem ser determinísticos. A propagação tem limite de profundidade defensivo, mas a validação da árvore pertence à futura camada de UI.
- `UINavigationRouter` implementa uma política simples de primeiro foco por `navigationOrder` e vizinho espacial; focus scopes, tab traversal, wrap-around e acessibilidade ainda não existem.
- O `NullRenderBackend` verifica a estrutura do pacote; não rasteriza pixels nem prova o comportamento de uma GPU.
- O shader está alinhado ao ABI, mas não foi compilado por DXC/FXC nem executado com Diligent nesta etapa.

## Critérios mínimos verificados

- ABI de GPU validada em compilação por `static_assert`.
- Builds GCC Debug e Release executados nesta etapa; Clang e AddressSanitizer/UndefinedBehaviorSanitizer não foram reexecutados nesta entrega.
- 130 verificações unitárias aprovadas, incluindo regressões de layout tree, remoção de subárvore, alinhamento e clipping/input.
- Exemplos de linha de comando (`spriteui_hello` e `spriteui_interaction_demo`) executam com o núcleo CPU/`NullRenderBackend`.
- Instalação CMake e consumo via `find_package` devem ser verificados no build final do pacote.

## Próxima ordem recomendada

1. Completar algoritmo Flex (freeze/re-distribuição min/max, wrap e justify), integração automática de layout → hit targets → clip indices.
2. Integração Flecs adapter e API de ciclo de frame.
3. Backend Diligent com ring buffers, múltiplos frames em voo e fences.
4. Window/input OS e widgets visuais.
5. Texto Unicode + HarfBuzz/FreeType/MSDF.
6. ToonMotion timeline e persistência JSON.
7. Integração Light Prism e efeitos.
8. Benchmarks, profiling em hardware real e testes visuais automatizados.

## Incremento 0.3.0 — Layout Tree + Clip Hierarchy (2026-11-04)

- Layout retido em duas fases (`Measure`/`Arrange`) com Row/Column, Fixed/Percent/Auto/Flex, grow/shrink simplificado, gap, padding, margens e min/max.
- `LayoutTree` usa relações explícitas de filhos; rejeita parent handles inválidos e remove subárvores recursivamente.
- Alinhamento transversal `Stretch`, `Start`, `Center` e `End`.
- `BuildClipHierarchy` calcula interseções de clips em ordem pai-antes-do-filho e fornece índices para `UIRenderItem::clipIndex`.
- `HitTarget` pode receber o clip efetivo em coordenadas lógicas; hit-test e estado `inside` no bubbling respeitam esse limite.
- 130 checks aprovados em GCC Debug e Release. DXC não está instalado; shaders não foram compilados.

Limitações restantes: Flexbox ainda não implementa wrap, justify-content, baseline, min-content/max-content nem algoritmo iterativo de freeze/re-distribuição min/max. A conversão automática da árvore de layout em hit targets e clip nodes permanece a próxima integração. O clipping é retangular e não cobre máscaras/rounded clips.
