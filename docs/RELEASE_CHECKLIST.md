# SpriteUi 0.6.0: checklist de publicação

Estado em **0.6.0-wip** (branch `feature/v0.6-text-wrap-flex-wrap`, 7 commits sobre o V4.2/0.5.1).

Legenda: ✅ feito **e verificado** neste ambiente · 🟡 feito, **não verificado** na toolchain alvo ·
⬜ pendente · ⛔ bloqueia a publicação.

**Ambiente da verificação:** GCC 13.3 / Linux, C++20, `-Wall -Wextra -Wpedantic -Wconversion -Wshadow`
(zero avisos), AddressSanitizer + UBSan, **727 checks** passando. **Não verificado:** MSVC, Clang,
CMake, compilação do HLSL, qualquer GPU.

---

## 1. O que foi feito nesta rodada

| # | Item | Evidência |
|---|---|---|
| ✅ | **Quebra de linha por palavra** (subconjunto do UAX #14: espaços, hífen, ZWSP, NBSP, CJK, clusters, emoji) | `Text/LineBreak.*`, `test_text_breaking.cpp` (fuzz de 4.500 entradas) |
| ✅ | **LayoutText com o texto de origem**: `\n`, espaços pendurados, quebra de emergência, alinhamento por largura de conteúdo | `test_text_layout.cpp`, teste diferencial contra o caminho legado |
| ✅ | **Caret por pen origin** (`GetCaretRect`) e **clique → ponto de inserção** (`FindInsertionOffset`) | `TestCaret*`, `TestInsertionOffsetRoundTrip` |
| ✅ | Bug: `GlyphInstance::yOffset` era ignorado no layout | `TestYOffsetRaisesTheGlyphBox` |
| ✅ | **Flexbox `wrap` + `alignContent`** (7 modos) e **`AlignItems::Baseline`** | `test_flex_wrap.cpp` + 4.000 árvores aleatórias idênticas ao motor antigo sem wrap |
| ✅ | Bug: estilo com NaN/Inf (margin, gap...) vazava para os retângulos | `TestNonFiniteGapAndSizesStayFinite` |
| ✅ | Otimização: alocações do solver de grow/shrink içadas para fora do laço | refatoração mecânica provada pelo diferencial |
| ✅ | **Nine-Slice** (§7 da especificação; **não existia no código**) com atenuação `kx, ky` | `test_nine_slice.cpp` (4.000 layouts, sem sobreposição) |
| ✅ | **Painel SDF arredondado**: referência em CPU + `cornerRadius` no item + flag nomeada | `TestSdfReferenceValues`, `TestVisualSdfPanels` |
| ✅ | Osciloscópio: `GetLatestSamples` (as N mais **recentes**) e proteção contra índice corrompido | `test_graph_sampling.cpp` |
| ✅ | **Robustez**: fuzz do parser JSON (6.000 mutações + 2.000 aleatórias), clip hostil, extrator corrompido | `test_robustness.cpp`: nenhuma falha encontrada |
| ✅ | **Regressão visual**: backend de software + PNG, 10 screenshots com asserção de pixels | `tests/Visual/`, `docs/img/` |
| ✅ | Constantes de flags nomeadas (o código usava `0x2`) | `Rendering/RenderFlags.hpp` |
| 🟡 | Shader: painel SDF (`FlagRoundedPanel`), `local`/`panel` interpolantes | `shaders/SpriteUI.hlsl` **não compilado** |
| 🟡 | `CMakeLists.txt` com os novos fontes/testes e `SPRITEUI_SNAPSHOT_DIR` | **não executado** (sem cmake no ambiente) |

### Mudanças de comportamento: nenhuma quebra de API
Todas as APIs existentes mantêm o comportamento (os 298 checks originais seguem passando). O que
poderia ser "corrigido" foi **adicionado ao lado** (`GetLatestSamples`, `GetCaretRect`,
`FindInsertionOffset`, `LayoutText(utf8, ...)`) e as versões antigas ficaram documentadas como legado.
**Decisão sua pendente:** quando marcar as antigas como `[[deprecated]]`.

---

## 2. Bloqueadores para publicar (⛔)

| # | Pendência | Por que bloqueia | Como fechar |
|---|---|---|---|
| ⛔ | **Compilar e rodar no alvo real**: CMake configure + build + `ctest`, **MSVC `/W4 /permissive-`** (alvo principal) e Clang | Só validei GCC. MSVC costuma achar `-Wconversion`-like, `std::span`/`<numbers>` e diferenças de float | `cmake --preset ...`; rodar `spriteui_tests` e `spriteui_check_headers` |
| ⛔ | **Compilar `SpriteUI.hlsl` (DXC)** e validar em GPU | Alterei o shader (SDF) sem compilador HLSL aqui | `dxc -T ps_6_0`; comparar com os PNGs de `docs/img/03..04` (paridade CPU↔GPU) |
| ⛔ | **CI** (GitHub Actions ou equivalente): GCC, Clang, MSVC + job ASan/UBSan | Hoje nada impede regressão; o repositório não tem `.github/` | matriz de 3 compiladores + sanitizers + `-Werror` |
| ⛔ | **Decidir o escopo da versão publicada**: "núcleo sem backend" ou "com Flecs/Diligent/HarfBuzz" | O README/§8 listam Flecs, Diligent, janela OS e HarfBuzz como pendentes | Se for núcleo: dizer isso no README e na descrição do pacote |
| ⛔ | **Sincronizar o documento normativo com o código** (ver §4) | O documento diz que a chave de ordenação é por material; o código (corretamente) não | aplicar a errata em `docs/ERRATA_V4_3.md` ao V4.4 |

## 3. Recomendado antes do 1.0 (⬜)

**Qualidade**
- ⬜ Benchmarks desta base (layout, extração, JSON, texto) com orçamento por frame; não foram medidos aqui
- ⬜ Fuzz contínuo do JSON (libFuzzer/OSS-Fuzz) com corpus; hoje o fuzz roda dentro dos testes
- ⬜ Cobertura de código (gcov/llvm-cov) e meta mínima
- ⬜ Testes de paridade CPU↔GPU (captura do backend real contra `docs/img`)
- ⬜ Teste de integração com **HarfBuzz/FreeType reais** (hoje só o `ShapeDebug` exercita o contrato `clusterByteOffset`)

**API e compatibilidade**
- ⬜ Revisão da API pública (nomes, `[[nodiscard]]`, `noexcept`, ordem de campos de `Edges`: `{left, top, right, bottom}` enganou duas vezes nos testes)
- ⬜ Política de versão semântica e de depreciação (0.x → 1.0); congelar a ABI de GPU (80 bytes, já com `static_assert`)
- ⬜ Documentar thread-safety (quais tipos são seguros entre threads)
- ⬜ `AlignContent` tem padrão `Start` (o CSS usa `stretch`) e o wrap usa um único `gap` para as duas direções: confirmar ou mudar

**Documentação e projeto**
- ⬜ `README.md` com screenshots (`docs/img/`), quick start e limites
- ⬜ `CHANGELOG.md`, `CONTRIBUTING.md`, `SECURITY.md` (política de relato de vulnerabilidades)
- ⬜ Referência de API (Doxygen) e guia de integração na engine
- ⬜ `THIRD_PARTY_NOTICES` quando entrarem Diligent (Apache-2.0), Flecs/QuickJS/HarfBuzz (MIT) e FreeType (FTL/GPLv2: escolher)
- ✅ `LICENSE`, `.clang-format`, `.editorconfig`, `CMakePresets.json` já existem
- ⬜ Pacote para vcpkg/Conan; `install(EXPORT)` já existe, falta testar `find_package(SpriteUI)` num projeto consumidor
- ⬜ Tag de release, notas de versão e artefato `.zip` limpo (sem `build/` nem `.cache/`)

## 4. Divergências encontradas entre o documento V4.3 e o código

| # | Documento | Código | Ação |
|---|---|---|---|
| 1 | §2/Spec: chave de 64 bits `Canvas│Layer│Clip│**Material**│Sibling`, ordenação por inteiro | `RenderExtractor` ordena por `(layer, zOrder, paintOrder)` e só agrupa itens **adjacentes** | O **código está certo**: o §2 inverte a ordem de pintura de translucentes (`docs/img/02_*`). Corrigir o documento |
| 2 | Matriz de QA: categorias somam **268** (42+65+48+38+45+30); título diz **298** | 298 *checks* (assertions) em ~16 funções de teste | Reescrever a matriz com a contagem real por suíte |
| 3 | QA cita 65 testes de "Flexbox & Nine-Slice" | Nine-Slice não existia | **Implementado nesta rodada** |
| 4 | QA cita testes de HarfBuzz/MSDF e validação MSVC/GCC/Clang | HarfBuzz pendente; só GCC verificado | Remover a afirmação ou marcá-la como alvo |
| 5 | §7 Spec: SDF `min(max(p_x−b_x+r, p_y−b_y+r), 0)` sem `|·|` e sem limitar o raio | `EvaluatePanelSdf` usa `abs` e limita `r ≤ min(halfSize)` | Corrigir a fórmula do documento |
| 6 | §2.1, §5: blocos de código corrompidos (`UIInputSnapshot`, `EvaluateShake`) | n/a | Restaurar a partir do código (`ToonMotion.cpp`) |
| 7 | §3 numeração de seções fora de ordem | n/a | Renumerar |

## 5. Limitações conhecidas desta rodada (documentar no README)

- Texto: sem **bidi/RTL** (ordem não monotônica cai no caminho legado), sem hifenização por idioma, sem kerning (é do HarfBuzz)
- Nine-Slice: só itens alinhados aos eixos (sem rotação/escala); não está ligado ao `LayoutBridge`/`ElementVisual`
- Painel SDF: só preenchimento + raio; **borda e sombra** precisam de espaço na ABI (usar `UIMaterialGPU`)
- Baseline: só em `Row`; em `Column` equivale a `Start`
- O backend de software dos testes **não** é o backend de produção: valida a matemática, não o driver

## 6. Caminho crítico sugerido

1. Rodar `cmake` + `ctest` em **MSVC e Clang** e corrigir o que aparecer
2. Compilar o HLSL e validar o painel SDF contra `docs/img/03_*` e `04_*`
3. Montar o **CI** (3 compiladores + sanitizers)
4. Decidir o escopo (núcleo vs. com backends) e ajustar README/versão
5. Aplicar a errata ao documento (V4.4) e fechar `README`/`CHANGELOG`/`CONTRIBUTING`/`SECURITY`
6. Medir benchmarks e fixar o orçamento por frame
7. Tag `v0.6.0` (ou `v1.0.0` se o escopo for só o núcleo)

## 7. Como reproduzir a verificação

```sh
g++ -std=c++20 -O1 -g -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
    -fsanitize=address,undefined -fno-sanitize-recover=all -Iinclude -Itests -Itests/Legacy \
    $(find src -name '*.cpp') tests/*.cpp tests/Legacy/LegacyLayout.cpp -o spriteui_tests
SPRITEUI_SNAPSHOT_DIR=snapshots ./spriteui_tests     # escreve os PNGs e imprime "All 727 checks passed."
```
