# Estado da implementação — SpriteUI 0.1.0 (Core MVP)

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
- Validador de `FramePacket`; `NullRenderBackend` rejeita pacotes incoerentes.
- Shader HLSL de referência e exemplo executável sem GPU.
- CMake com targets de instalação e `find_package(SpriteUI CONFIG)`.

## Pendente — não declarado como implementado

| Módulo | Estado |
|---|---|
| Flecs ECS adapter | Pendente |
| Diligent Engine backend e gestão real de buffers/fences | Pendente |
| Compilação/validação de HLSL por DXC | Não validado neste ambiente (DXC ausente) |
| Flexbox completo, Measure/Arrange recursivo, min/max e distribuição iterativa | Pendente |
| Construção de árvore de UI, Canvas/Scene graph e clipping hierárquico | Pendente |
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
- Builds GCC Debug/Release, Clang e AddressSanitizer/UndefinedBehaviorSanitizer (quando configurados) executados durante o desenvolvimento.
- 111 verificações unitárias para transformações, hit-testing transformado, foco/captura, cancelamento, bubbling, navegação semântica, widgets, animações, batching e validação de pacote.
- Exemplos de linha de comando (`spriteui_hello` e `spriteui_interaction_demo`) executam com o núcleo CPU/`NullRenderBackend`.
- Instalação CMake e consumo via `find_package` devem ser verificados no build final do pacote.

## Próxima ordem recomendada

1. Flex layout + árvore de UI + clipping hierárquico.
2. Integração Flecs adapter e API de ciclo de frame.
3. Backend Diligent com ring buffers, múltiplos frames em voo e fences.
4. Window/input OS e widgets visuais.
5. Texto Unicode + HarfBuzz/FreeType/MSDF.
6. ToonMotion timeline e persistência JSON.
7. Integração Light Prism e efeitos.
8. Benchmarks, profiling em hardware real e testes visuais automatizados.

## Continuação — UI Core / Layout (2026-11-04)

Implementado primeiro incremento da recomendação: engine de layout retido em duas fases (`Measure`/`Arrange`) e árvore de layout por `UIHandle`.

- `LayoutEngine`: Row/Column, Fixed/Percent/Auto/Flex, grow/shrink, gap, padding, margins, min/max.
- `LayoutTree`: criação/remoção de nós, atualização de estilo/visibilidade e layout de roots.
- `LayoutStats`: contadores de nós medidos e organizados para instrumentação.
- Teste de regressão para distribuição de espaço em Row com gap e grow.

Limitações deste incremento: ainda não é Flexbox completo (wrap, align/justify, baseline, min-content/max-content e algoritmo iterativo de freeze/shrink ainda não implementados); árvore usa descoberta por varredura do mapa e deve ser otimizada com listas de filhos explícitas antes de workloads grandes. O layout está independente de Flecs e backend.
