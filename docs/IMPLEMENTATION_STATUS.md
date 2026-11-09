# Estado da implementação — SpriteUI 0.6.0 (verificado em 09/11/2026)

O CMake declara a versão 0.6.0; isso não significa que uma release esteja publicada. O núcleo C++20 segue independente de ECS/RHI. A arquitetura planejada não deve ser confundida com integrações já entregues.

## Correções de Formatação, Estrutura e Insuficiência de Código

- **Padronização de Formato:** Os arquivos `Layout.hpp`, `Layout.cpp`, `LayoutTree.hpp` e `LayoutTree.cpp`, que estavam compactados/sem formatação legível, foram integralmente reformatados para o padrão C++20 da biblioteca, com tipagem estrita (`std::uint8_t`, `std::uint32_t`), indentação limpa de 4 espaços e documentação clara.
- **Eliminação de Avisos de Compilação:** Resolução de todos os avisos C4100 (parâmetros não referenciados) e C4834 (`[[nodiscard]]` descartado), garantindo compilação limpa em nível `/W4 /permissive-`.
- **Limpeza de Diretórios Vazios:** O diretório órfão `src/Core` foi removido.
- **Expansão de Flexbox:** `JustifyContent`, wrap, `AlignContent`, baseline e redistribuição com limites min/max estão implementados.

## Módulos Implementados

- **ABI e GPU:**
  - ABI `SpriteInstanceGPU` de 80 bytes, alinhamento 16 e verificações de offsets/tipos por `static_assert`.
  - `UIMaterialGPU` base e configuração de Canvas Screen/World com validação do modo de iluminação.
  - Render extraction com ordenação visual estável, batches apenas consecutivos, clip rects e ABI de 80 bytes.
  - Validador de `FramePacket`; `NullRenderBackend` para execução e validação sem GPU.
  - Shader HLSL (`SpriteUI.hlsl`) com amostragem MSDF por `fwidth` e compilação offline DXIL ativa.

- **Layout e Cena:**
  - Flexbox com Row/Column, Fixed/Percent/Auto/Flex, grow/shrink, gap, padding, margens, min/max, `JustifyContent`, wrap, `AlignContent` e baseline.
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
  - `GlyphInstance`, `FontMetrics`, `LayoutText` (quads, quebra por palavra no subconjunto implementado, alinhamento), `GetCaretRect`, `FindInsertionOffset`, `FindClusterAtPosition` e `GenerateTextRenderItems`.
  - HarfBuzz/FreeType, atlas de produção e bidi/RTL não estão integrados.
- **Renderização auxiliar:** Nine-Slice e painéis SDF arredondados; compilação DXIL do shader verificada localmente, sem validação em GPU real.

- **Persistência Segura (JSON):**
  - `UIConfigManager` com validação de `schemaVersion`, sanitização numérica por `ReadFiniteFloat`, staging atômico e commit transacional.

- **Verificação local (MSVC/Windows):**
  - Builds Debug e Release, `spriteui_check_headers`, CTest (core + instalação/consumidor) e exemplos CLI passaram.
  - `spriteui_tests.exe` reportou 727 checks aprovados em Debug; o CTest passou nas duas configurações.
  - DXC compilou os shaders VS/PS para DXIL.
  - Clang/GCC, sanitizers, CI e GPU real não foram verificados nesta rodada.
- **Preparação para release adicionada:** workflow CI para MSVC/GCC/Clang e ASan/UBSan; teste CTest de instalação/consumidor `find_package`; benchmark CPU opcional de layout e render extraction. O smoke test de pacote e benchmark foram executados localmente; o workflow remoto ainda precisa executar com sucesso no GitHub Actions.
- **Documentação de adoção:** README com quick start e screenshots de software; `CHANGELOG.md`, `CONTRIBUTING.md`, `SECURITY.md` e notas de thread-safety adicionados. Referência de API e guia completo de integração ainda pendentes.
- **Integração opcional SDL3:** adaptador de eventos de mouse/foco para `UIInputSnapshot`, selecionável por `SPRITEUI_BUILD_SDL3_ADAPTER`; compilado localmente com SDL 3.2.30, e CTest (core + pacote consumidor) passou. O workflow também ganhou um job com SDL 3.2.30 fixado; a execução remota ainda não foi confirmada.

## Pendências para integração e produção

| Módulo | Estado |
|---|---|
| Flecs ECS adapter | Pendente |
| Diligent Engine backend, submissão GPU, buffers e fences | Pendente |
| Adaptador SDL3 para mouse/foco | Implementado e validado localmente com SDL 3.2.30; job CI remoto pendente |
| Criação/gestão de janela, clipboard, IME, input multi-pointer e alternativa GLFW | Pendente |
| HarfBuzz/FreeType, atlas MSDF dinâmico e suporte bidi/RTL | Pendente |
| Widgets visuais integrados e drag-and-drop de docking | Pendente |
| Light Prism ClusterLit, normal mapping, Acrylic Blur, Bloom e sombras | Pendente |
| KTX2/Basis, streaming e eviction de recursos por fences | Pendente |
| ToonMotion timeline/keyframes e integração ECS | Pendente |
| Scheduler on-demand baseado em dirty flags e orçamento de desempenho em engine hospedeira | Pendente |
| QuickJS sandbox/FFI, partículas e nodes de editor | Pendente |
| CI configurado para MSVC/Clang/GCC e job ASan/UBSan | Implementado; falta observar execução remota em GitHub Actions |
| Fuzz contínuo e cobertura | Pendente |
| Referência de API (Doxygen) e guia de integração na engine | Pendente |
| Benchmarks CPU de layout e render extraction | Implementado como alvo opcional e executado localmente; faltam baseline comparável e orçamento por plataforma |
| Teste de instalação/consumidor com `find_package` | Implementado como teste CTest e aprovado localmente em MSVC Debug e Release; falta validar em outras plataformas/toolchains |
| Validação em GPU real | Pendente |
