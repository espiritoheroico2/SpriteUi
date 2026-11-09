# SpriteUi 0.6.0: checklist de publicação

Estado em **0.6.0**, sem release publicada. A versão declarada no CMake não é uma tag de release.

Legenda: ✅ feito **e verificado** neste ambiente · 🟡 feito, **não verificado** na toolchain alvo ·
⬜ pendente · ⛔ bloqueia a publicação.

**Ambiente verificado nesta rodada:** Windows, MSVC 14.51, CMake/Ninja Debug e Release,
`spriteui_check_headers`, CTest (core + pacote consumidor), exemplos CLI, DXC VS/PS e **727 checks**
passando em Debug. O benchmark CPU também foi executado localmente nas duas configurações. O
adaptador SDL3 e o pacote instalado passaram com SDL 3.2.30.
**Não verificado nesta rodada:** Clang/GCC, sanitizers, CI remoto e qualquer GPU real.

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
| ✅ | Shader HLSL de referência | Compilado com DXC para VS/PS DXIL; execução e paridade em GPU continuam pendentes |
| ✅ | Build CMake e alvo de headers públicos | Build Debug MSVC, `spriteui_check_headers` e CTest executados |
| ✅ | Build de produção MSVC Release | Build Release, checagem de headers, CTest e package smoke test passaram |
| ✅ | CI multiplataforma configurado | `.github/workflows/ci.yml`: MSVC, GCC, Clang, job ASan/UBSan e build do adaptador SDL3 com SDL 3.2.30; execução remota pendente |
| ✅ | Smoke test de pacote instalado | CTest instala a biblioteca e compila consumidor independente via `find_package`; aprovado em MSVC Debug e Release |
| ✅ | Benchmarks CPU básicos | Alvo opcional executado localmente em Debug e Release; baseline e budgets pendentes |
| ✅ | Adaptador de input SDL3 opcional | Compilado com SDL 3.2.30; CTest (core + pacote consumidor) aprovado localmente |

### Mudanças de comportamento: nenhuma quebra de API
Todas as APIs existentes mantêm o comportamento; o executável reportou 727 checks aprovados. O que
poderia ser "corrigido" foi **adicionado ao lado** (`GetLatestSamples`, `GetCaretRect`,
`FindInsertionOffset`, `LayoutText(utf8, ...)`) e as versões antigas ficaram documentadas como legado.
**Decisão sua pendente:** quando marcar as antigas como `[[deprecated]]`.

---

## 2. Bloqueadores para publicar (⛔)

| # | Pendência | Por que bloqueia | Como fechar |
|---|---|---|---|
| ⛔ | **Completar matriz de toolchains**: Clang e GCC, além de sanitizers | MSVC Debug e Release foram verificados; outras toolchains não foram verificadas nesta rodada | Rodar configure/build/test/header checks em Clang/GCC e sanitizers |
| ⛔ | **Validar HLSL em GPU real** | DXC gerou os shaders; ainda não há backend nem teste de paridade CPU↔GPU | Implementar backend e comparar resultados com `docs/img/03..04` |
| ⛔ | **Executar e estabilizar o CI** | Workflow foi adicionado, mas ainda não há resultado remoto confirmado | Executar no GitHub Actions, corrigir falhas e manter a matriz verde |
| ⛔ | **Decidir o escopo da versão publicada**: "núcleo sem backend" ou "com Flecs/Diligent/HarfBuzz" | O README/§8 listam Flecs, Diligent, janela OS e HarfBuzz como pendentes | Se for núcleo: dizer isso no README e na descrição do pacote |
| ⛔ | **Sincronizar toda a documentação de arquitetura com o código** | Os textos normativos ainda contêm contratos e exemplos históricos que não correspondem à entrega | Manter status de implementação e limites explícitos nos documentos e revisar a especificação antes de uma nova versão |

## 3. Recomendado antes do 1.0 (⬜)

**Qualidade**
- ⬜ Baselines e orçamento por frame para benchmarks (alvo atual cobre layout e extração; JSON/texto ainda faltam)
- Referência local (Windows/MSVC; 250 iterações, 1.000 filhos/itens): Debug — layout 1,90 ms/frame, extração 1,70 ms/frame; Release — layout 0,066 ms/frame, extração 0,205 ms/frame. Medições indicativas, sem controle de ruído ou meta de aprovação.
- ⬜ Fuzz contínuo do JSON (libFuzzer/OSS-Fuzz) com corpus; hoje o fuzz roda dentro dos testes
- ⬜ Cobertura de código (gcov/llvm-cov) e meta mínima
- ⬜ Testes de paridade CPU↔GPU (captura do backend real contra `docs/img`)
- ⬜ Teste de integração com **HarfBuzz/FreeType reais** (hoje só o `ShapeDebug` exercita o contrato `clusterByteOffset`)

**API e compatibilidade**
- ⬜ Revisão da API pública (nomes, `[[nodiscard]]`, `noexcept`, ordem de campos de `Edges`: `{left, top, right, bottom}` enganou duas vezes nos testes)
- ⬜ Política de versão semântica e de depreciação (0.x → 1.0); congelar a ABI de GPU (80 bytes, já com `static_assert`)
- ✅ Notas de thread-safety do core documentadas; contratos de adaptadores/backends futuros permanecem pendentes
- ⬜ `AlignContent` tem padrão `Start` (o CSS usa `stretch`) e o wrap usa um único `gap` para as duas direções: confirmar ou mudar

**Documentação e projeto**
- ✅ `README.md` com screenshots do backend de software, quick start e limites
- ✅ `CHANGELOG.md`, `CONTRIBUTING.md`, `SECURITY.md` e notas de thread-safety adicionados
- ⬜ Referência de API (Doxygen) e guia de integração na engine
- ⬜ `THIRD_PARTY_NOTICES` quando entrarem Diligent (Apache-2.0), Flecs/QuickJS/HarfBuzz (MIT) e FreeType (FTL/GPLv2: escolher)
- ✅ `LICENSE`, `.clang-format`, `.editorconfig`, `CMakePresets.json` já existem
- ⬜ Pacote para vcpkg/Conan; `install(EXPORT)` já existe, falta testar `find_package(SpriteUI)` num projeto consumidor
- ⬜ Tag de release, notas de versão e artefato `.zip` limpo (sem `build/` nem `.cache/`)

## 4. Divergências encontradas entre o documento V4.3 e o código

| # | Documento | Código | Ação |
|---|---|---|---|
| 1 | A especificação sugere ordenar por material | `RenderExtractor` ordena por `(layer, zOrder, paintOrder)` e só agrupa itens adjacentes | Preservar a ordem visual do código; atualizar a especificação antes de tratá-la como contrato de batching |
| 2 | Matriz de QA e quantidades por categoria são históricas e inconsistentes | A execução atual reporta 727 checks totais; não foi validada uma contagem por categoria | Não publicar números por suíte sem recalcular a matriz |
| 3 | Estado antigo diz que Flex wrap, quebra por palavra e Nine-Slice não existem | Essas capacidades e testes estão no código atual | Estado atualizado neste checklist; manter os documentos de arquitetura sincronizados |
| 4 | Alegações de integração HarfBuzz/FreeType e validação em múltiplas toolchains | Shaper de debug existe; integração real de fontes pendente. MSVC/DXC foram verificados; Clang/GCC não nesta rodada | Distinguir código auxiliar de integração de produção e registrar a toolchain usada |
| 5 | Contratos de SDF e exemplos de input/animação diferem do código atual | Implementações de referência estão em `PanelSdf`, `InputRouter` e `ToonMotion` | Revisar os exemplos e fórmulas normativos antes de uma nova versão da especificação |
| 6 | Seções e histórico misturam material legado e contratos atuais | O documento contém versões e fases de planejamento anteriores | Identificar explicitamente conteúdo histórico e manter a seção de status atualizada |

## 5. Limitações conhecidas desta rodada

- Texto: sem integração HarfBuzz/FreeType nem bidi/RTL ou hifenização por idioma; o core aceita glifos shaped e não faz shaping de produção.
- Nine-Slice: cálculo para itens alinhados aos eixos, sem rotação/escala; ainda não ligado ao `LayoutBridge`/`ElementVisual`.
- Painel SDF: referência para preenchimento arredondado; bordas, sombras e efeitos continuam pendentes.
- Baseline: implementado em `Row`; em `Column` equivale a `Start`.
- O backend de software dos testes **não** é o backend de produção: valida a matemática, não o driver.
- O adaptador SDL3 limita-se a eventos de mouse/foco; criação da janela, teclado/gamepad, clipboard e IME não fazem parte deste primeiro passo.

## 6. Caminho crítico sugerido

1. Decidir e documentar o escopo de release (core CPU vs. UI com integrações)
2. Implementar backend Diligent e validar HLSL/paridade em GPU real
3. Implementar adaptadores ECS/plataforma conforme o escopo escolhido
4. Completar matriz Clang/GCC/MSVC, Release, sanitizers e CI
5. Testar instalação/exportação com um projeto consumidor e `find_package`
6. Medir benchmarks, documentar limites, thread-safety e política de versão
7. Preparar CHANGELOG, instruções de contribuição/segurança, tag e artefato limpo

## 7. Como reproduzir a verificação

```sh
g++ -std=c++20 -O1 -g -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
    -fsanitize=address,undefined -fno-sanitize-recover=all -Iinclude -Itests -Itests/Legacy \
    $(find src -name '*.cpp') tests/*.cpp tests/Legacy/LegacyLayout.cpp -o spriteui_tests
SPRITEUI_SNAPSHOT_DIR=snapshots ./spriteui_tests     # escreve os PNGs e imprime "All 727 checks passed."
```
