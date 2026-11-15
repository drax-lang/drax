/* dmr - drax manager & resolver
 *
 * `dmr graph`: the AI part of dmr. It reads .dx sources and writes a compact,
 * deterministic code property graph for LLMs to the .drax-cpg/ folder.
 * It has its own scanner: it does not depend on the interpreter, so this
 * file and dlang.* can move together to another repository.
 * See docs/graph.md.
 */
#ifndef __DMR_GRAPH
#define __DMR_GRAPH

#define DMR_GRAPH_DIR ".drax-cpg"

/* paths: files and/or directories (recursive). returns the exit code. */
int cmd_graph(int npaths, char** paths);

#endif
