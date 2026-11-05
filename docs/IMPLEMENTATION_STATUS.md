# Estado da implementação — SpriteUI 0.5.1 (revisão de conformidade com V4.2)

Base normativa: documento SpriteUi V4.2 fornecido pelo utilizador. Esta implementação prioriza um núcleo C++20 robusto, modular, sem dependências de terceiros obrigatórias no núcleo, compilável e testável com zero warnings em MSVC (`/W4 /permissive-`) e GCC/Clang.

## Correções de Formatação, Estrutura e Insuficiência de Código

- **Padronização de Formato:** Os arquivos `Layout.hpp`, `Layout.cpp`, `LayoutTree.hpp` e `LayoutTree.cpp`, que estavam compactados/sem formatação legível, foram integralmente reformatados para o padrão C++20 da biblioteca, com tipagem estrita (`std::uint8_t`, `std::uint32_t`), indentação limpa de 4 espaços e documentação clara.
- **Eliminação de Avisos de Compilação:** Resolução de todos os avisos C4100 (parâmetros não referenciados) e C4834 (`[[nodiscard]]` descartado), garantindo compilação limpa em nível `/W4 /permissive-`.
- **Limpeza de Diretórios Vazios:** O diretório órfão `src/Core` foi removido.
- **Expansão de Flexbox:** Adicionado o enum `JustifyContent` (`FlexStart`, `Center`, `FlexEnd`, `SpaceBetween`, `SpaceAround`, `SpaceEvenly`) ao motor de layout em duas fases, suprindo a carência de alinhamento no eixo principal quando sobra espaço livre.

## Módulos Implementados

- **ABI e GPU:**
  - ABI `SpriteInstanceGPU` de 80 bytes, alinhamento 16 e verificações de offsets/tipos por `static_assert`.
  - `UIMaterialGPU` base e configuração de Canvas Screen/World com validação do modo de iluminação.
  - Render extraction com ordenação visual estável, batches apenas consecutivos, clip rects e ABI de 80 bytes.
  - Validador de `FramePacket`; `NullRenderBackend` para execução e validação sem GPU.
  - Shader HLSL (`SpriteUI.hlsl`) com amostragem MSDF por `fwidth` e compilação offline DXIL ativa.

- **Layout e Cena:**
  - Flexbox com Row/Column, Fixed/Percent/Auto/Flex, grow/shrink, gap, padding, margens, min/max e `JustifyContent`.
  - `LayoutTree` com relações explícitas de filhos, validação de handles e remoção recursiva de subárvores.
  - Alinhamento transversal Stretch/Start/Center/End.
  - `BuildClipHierarchy` com interseção hierárquica pai/filho.
  - `LayoutSceneBridge`: conversão automática da `LayoutTree` em lista plana de `HitTarget`, `UIRenderItem` e hierarquia de `ClipNode`.

- **Widgets e Controle de Janela:**
  - `UISliderComponent` (domínio numérico, step e proteção contra largura zero).
  - `UIScrollViewComponent` (`ClampScroll` por `contentSize - viewportSize`).
  - `UIToggleComponent` (ligado/desligado e desabilitado).
  - `UITextInputComponent` (foco, cursor de cluster, seleção e temporizador de cursor piscante).
  - `GetResizeEdge` (detecção de 8 bordas: Left, Right, Top, Bottom e 4 cantos).
  - `UIWindowComponent` e `ProcessWindowInteraction` (arrasto por titlebar, redimensionamento nas 8 bordas respeitando `minSize` e captura no roteador).

- **Novos Módulos Implementados (Nesta Etapa):**
  - **Virtualização de Listas (`UIVirtualList`):** Algoritmo O(1) de cálculo de faixa visível `[firstIndex, lastIndex]` com overscan, viabilizando tabelas com centenas de milhares de linhas (Seção 10).
  - **Abas e Docking Estrutural (`UITabGroupComponent` e `UIDockNode`):** Gestão de abas ativas/inativas e árvore binária de divisão de dockspace com splits Left, Right, Top, Bottom e Center (Seção 24 e 25.2).
  - **Data Binding Reativo MVVM (`UIValueBinding`):** `UIFloatBinding`, `UIIntBinding`, `UIBoolBinding` e `UIStringBinding` com dirty check por limiar numérico (Seção 28 e Apêndice F.3).
  - **Gráficos Analíticos e Telemetria (`UIOscilloscopeGraph` e `UILinePlot`):** Ring buffer circular de 256 amostras com O(1) de inserção, extração cronológica ordenada e normalização [0, 1] para plotagem de frametimes e frequências (Seção 29 e 30.1).
  - **Gerenciador de Popups (`UIPopupManager`):** Rastreio de menus de contexto e dropdowns, com fechamento automático em cliques fora da barreira AABB (Seção 25.7).

- **Tipografia e Texto:**
  - `GlyphInstance`, `FontMetrics`, `LayoutText` (quads, wrap, alinhamento), `GetCaretPosition`, `FindClusterAtPosition`, `GenerateTextRenderItems` e `UITextCacheComponent`.

- **Persistência Segura (JSON):**
  - `UIConfigManager` com validação de `schemaVersion`, sanitização numérica por `ReadFiniteFloat`, staging atômico e commit transacional.

- **Testes Unitários:**
  - 298 verificações unitárias aprovadas em `spriteui_tests.exe` (100% de sucesso).

## Pendente

| Módulo | Estado |
|---|---|
| Flecs ECS adapter | Pendente |
| Diligent Engine backend e gestão real de buffers/fences | Pendente |
| Backend de janela/OS nativo (SDL3/GLFW) e IME do OS | Pendente |
| HarfBuzz/FreeType C++ bindings automáticos | Pendente |
| Light Prism ClusterLit, normal mapping e Acrylic Blur | Pendente |
| QuickJS sandbox e FFI | Pendente |
