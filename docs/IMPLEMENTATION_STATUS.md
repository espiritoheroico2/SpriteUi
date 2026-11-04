# Estado da implementação — SpriteUI 0.4.0 (Text Pipeline, Window Interaction, Scene Bridge & Safe JSON)

Base normativa: documento SpriteUi V4.2 fornecido pelo utilizador. Esta implementação prioriza um núcleo C++20 robusto, modular, sem dependências de terceiros obrigatórias no núcleo, compilável e testável com zero warnings em MSVC (`/W4 /permissive-`) e GCC/Clang.

## Implementado no projeto

- **ABI e GPU:**
  - ABI `SpriteInstanceGPU` de 80 bytes, alinhamento 16 e verificações de offsets/tipos por `static_assert`.
  - `UIMaterialGPU` base e configuração de Canvas Screen/World com validação do modo de iluminação.
  - Render extraction com ordenação visual estável, batches apenas consecutivos, clip rects e ABI de 80 bytes.
  - Validador de `FramePacket`; `NullRenderBackend` para execução e validação sem GPU.
  - Shader HLSL (`SpriteUI.hlsl`) atualizado com suporte a amostragem MSDF e derivadas analíticas de UV (`fwidth`).
  - Compilação offline de shaders HLSL para DXIL (`SpriteUI.vs.dxil` e `SpriteUI.ps.dxil`) via DXC ativa e funcional no build.

- **Matemática e Transformações:**
  - Matemática 2D (`Vec2`, `Vec4`, `Rect`, `Affine2D`), anchors/offsets, transformação de quad e transformação local em pixels para hit-testing com pivô, rotação e escala.
  - Handles lógicos com índice + geração (`UIHandle`).
  - Flags de invalidação por subsistema (`UIDirtyBits` e `UIDirtyState`).

- **Input e Roteamento:**
  - Input snapshot (`UIInputSnapshot`), hit-testing inverso por paint order, foco, captura, hover, click, wheel, cancelamento por perda de foco da janela e proteção contra `frameId` repetido.
  - Propagação de eventos aos ancestrais por `HitTarget::parent`; o consumo de input considera bloqueio no alvo ou num ancestral (`blocksInput`).
  - Navegação espacial e roteador semântico para `Activate`/`Cancel`.

- **Layout e Cena:**
  - Layout retido em duas fases (`Measure`/`Arrange`) com Row/Column, Fixed/Percent/Auto/Flex, grow/shrink simplificado, gap, padding, margens e min/max.
  - `LayoutTree` com relações explícitas de filhos, validação de parent handle e remoção recursiva de subárvores.
  - Alinhamento transversal Stretch/Start/Center/End.
  - `BuildClipHierarchy` com interseção hierárquica pai/filho.
  - `LayoutSceneBridge`: conversão automática da `LayoutTree` em lista plana de `HitTarget`, `UIRenderItem` e hierarquia de `ClipNode` para extração de GPU.

- **Widgets e Interação de Janela:**
  - Slider com validação de domínio numérico, passo opcional e proteção contra largura zero.
  - ScrollView com clamp por `contentSize - viewportSize`.
  - Toggle Switch (`UIToggleComponent`) com estados ligado/desligado e desabilitado.
  - Text Input (`UITextInputComponent`) com controle de foco, cursor de cluster, intervalo de seleção e temporizador de cursor piscante (`blinkTimer`).
  - Detecção de 8 bordas de redimensionamento (`GetResizeEdge`: Left, Right, Top, Bottom, TopLeft, TopRight, BottomLeft, BottomRight).
  - Interação de janelas (`UIWindowComponent` e `ProcessWindowInteraction`): arrasto por titlebar, redimensionamento nas 8 bordas respeitando `minSize`, e captura de foco/ponteiro no roteador.

- **Tipografia e Texto:**
  - Estrutura de dados `GlyphInstance` alinhada a HarfBuzz e MSDF.
  - `FontMetrics`: ascent, descent, lineGap, unitsPerEm, pxRange e cálculo de escala/altura de linha.
  - `LayoutText`: cálculo geométrico de quads para glifos shaped, quebra de linha com `maxWidth`, e alinhamentos `Left`, `Center`, `Right`.
  - `GetCaretPosition`: cálculo de coordenadas locais (X, Y) do cursor de texto por cluster byte offset.
  - `FindClusterAtPosition`: detecção do cluster de caractere mais próximo da coordenada do clique.
  - `GenerateTextRenderItems`: geração direta de `UIRenderItem` com flags de MSDF para extração de GPU.
  - `UITextCacheComponent`: cache de layout de texto com sinalizadores de invalidação (`isDirty`).

- **Persistência Segura (JSON):**
  - Implementação normativa da Seção 7 do documento V4.2 (`UIConfigManager`).
  - Validação estrita de cabeçalho `schemaVersion` (rejeita schemas ausentes ou incompatíveis).
  - Sanitização de valores numéricos por `ReadFiniteFloat` contra valores ausentes, infinitos e NaN.
  - Pipeline transacional em dois estágios: `ValidateAndStage` (validação atômica) e `Commit` (aplicação definitiva).
  - Serialização JSON padronizada e round-trip testado.

- **Animações (ToonMotion):**
  - Motor ToonMotion V2 com easing, políticas de conflito `Override`, `Queue`, `Ignore`, pausa/cancelamento e shake não cumulativo (`EvaluateShake`).

- **Testes e Build:**
  - 211 verificações unitárias aprovadas em `spriteui_tests.exe`.
  - Exemplos executáveis `spriteui_hello` e `spriteui_interaction_demo`.

## Pendente — não declarado como implementado

| Módulo | Estado |
|---|---|
| Flecs ECS adapter | Pendente |
| Diligent Engine backend e gestão real de buffers/fences | Pendente |
| Backend de janela/OS nativo (SDL3/GLFW) e IME do OS | Pendente |
| HarfBuzz/FreeType bindings nativos (shaper automático a partir de string UTF-8) | Pendente; pipeline de layout de glifos shaped e quads já implementado |
| ToonMotion tracks persistentes acopladas em Flecs | Pendente; motor lógico C++ já funcional |
| Light Prism ClusterLit, normal mapping e Acrylic Blur | Pendente |
| Partículas 2D e gráficos (Line/Oscilloscope) | Pendente |
| QuickJS sandbox e FFI | Pendente |
