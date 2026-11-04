# Mapeamento da especificação V4.2 para o código-fonte

Este mapa impede que propostas documentais sejam confundidas com funcionalidades já executáveis.

| Contrato V4.2 | Código / Estado |
|---|---|
| ABI `SpriteInstanceGPU` 80 bytes | `include/SpriteUi/Rendering/SpriteInstanceGPU.hpp` — implementado e validado por `static_assert` |
| Screen/World + Unlit/ClusterLit | `Rendering/CanvasConfig.hpp` — configuração e validação lógica; consumo real da Light Prism pendente |
| Input snapshot, foco e captura | `Input/InputRouter.hpp`, `src/Input/InputRouter.cpp` — implementado para um ponteiro primário |
| Eventos semânticos e bubbling | `UIEventRouter` — bubbling pelos `parent` fornecidos; sem árvore de UI automática |
| Navegação Activate/Cancel | `Input/SpatialNavigation.hpp` — roteador semântico básico implementado |
| Slider e ScrollView | `Widgets/Widgets.hpp` — lógica e clamp implementados; renderização visual pendente |
| GlyphInstance | `Text/GlyphInstance.hpp` — estrutura de dados inicial; shaping/atlas pendentes |
| ToonMotion V2 | `Animation/ToonMotion.hpp`, `src/Animation/ToonMotion.cpp` — tracks básicas e políticas de conflito; timeline/keyframes/ECS pendentes |
| Dirty flags | `Core/DirtyFlags.hpp` — estado e operações implementados; propagação automática pela árvore pendente |
| Extração GPU e batching | `Rendering/DrawList.hpp`, `src/Rendering/DrawList.cpp` — extração CPU e batching adjacente implementados |
| Shader Unlit | `shaders/SpriteUI.hlsl` — referência alinhada; DXC e backend reais pendentes |
| Flecs | Adaptador ainda não existe |
| Diligent | Interface `IRenderBackend` e `NullRenderBackend` existem; backend Diligent não existe |
| Flexbox, docking, tabs, drag & drop | Pendente |
| JSON transacional | Pendente; o ConfigManager do ZIP não foi incorporado por trazer dependência nlohmann/json não declarada nem pipeline transacional completo |
| HarfBuzz/MSDF/IME | Pendente |
| KTX2/Basis/resource fences | Pendente |
| Light Prism, Bloom, Blur e partículas | Pendente |
| QuickJS/FFI | Pendente |

## Tratamento do arquivo `SpriteUI_V4_2_Library.zip`

O ZIP foi inspecionado. Ele contém um esqueleto alternativo com apenas nove ficheiros, caminhos `include/SpriteUI` (capitalização diferente de `include/SpriteUi`), um CMake que compila apenas `src/Core/SpriteUI.cpp`, opções de Flecs/Diligent não conectadas ao build e um header de configuração que inclui `nlohmann/json.hpp` sem declarar/localizar essa dependência. Os contratos úteis (ABI de 80 bytes, estruturas de input/widgets, shake e shader Unlit) foram comparados com a base atual. Não foi copiado por cima da base porque isso criaria duas árvores públicas e símbolos paralelos; os contratos compatíveis já estão representados na árvore canónica `include/SpriteUi`.
