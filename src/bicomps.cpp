/*
 * Usage:
 *  bicomps bc(n);
 *  for ...
 *    bc.add_edge(u, v);
 *  bc.build();
 */
struct bicomps {
  struct edge {
    int node, index;

    edge() {}

    edge(int _node, int _index) : node(_node), index(_index) {}
  };

  int n, m;
  vector<vector<edge>> adj;
  vector<array<int, 2>> edge_list;
  vector<int> tour_start;
  vector<int> low_link;

  vector<bool> vis;
  vector<bool> is_cut;
  vector<bool> is_bridge;
  vector<int> stk;
  vector<vector<int>> comps;
  int tour;

  bicomps(int _n = 0, int _m = 0) {
    init(_n, _m);
  }

  void init(int _n, int _m = 0) {
    n = _n;
    m = _m;
    adj.assign(n, {});
    edge_list.clear();
    edge_list.reserve(m);
    tour_start.resize(n);
    low_link.resize(n);
  }

  void add_edge(int u, int v) {
    adj[u].emplace_back(v, m);
    adj[v].emplace_back(u, m);
    edge_list.push_back({u, v});
    ++m;
  }

  // This can only be called once per node.
  void dfs(int node, int parent) {
    assert(!vis[node]);
    vis[node] = true;
    tour_start[node] = tour++;
    low_link[node] = tour_start[node];
    is_cut[node] = false;
    int parent_count = 0, children = 0;

    for (edge &e : adj[node]) {
      // Skip the first edge to the parent, but allow multi-edges.
      if (e.node == parent && parent_count++ == 0)
        continue;

      if (vis[e.node]) {
        // e.node is a candidate for low_link.
        low_link[node] = min(low_link[node], tour_start[e.node]);

        // Make sure we only add it in one direction.
        if (tour_start[e.node] < tour_start[node])
          stk.push_back(node);
      } else {
        int size = int(stk.size());
        dfs(e.node, node);
        ++children;

        // e.node is part of our subtree.
        low_link[node] = min(low_link[node], low_link[e.node]);

        if (low_link[e.node] > tour_start[node]) {
          // This is a bridge.
          is_bridge[e.index] = true;
          vector<int> comp = {node, e.node};
          sort(comp.begin(), comp.end());
          comps.push_back(comp);
        } else if (low_link[e.node] == tour_start[node]) {
          // This is the root of a biconnected comp.
          stk.push_back(node);
          vector<int> comp(stk.begin() + size, stk.end());
          sort(comp.begin(), comp.end());
          comp.erase(unique(comp.begin(), comp.end()), comp.end());
          comps.push_back(comp);
          stk.resize(size);
        } else {
          stk.push_back(node);
        }

        // In general, `node` is a cut vertex iff it has a child whose subtree cannot reach above `node`.
        // Note that this is not true of the root; it is handled specially below.
        if (low_link[e.node] >= tour_start[node])
          is_cut[node] = true;
      }
    }

    // The root of the tree is a cut vertex iff it has more than one child.
    if (parent < 0)
      is_cut[node] = children > 1;
  }

  void build(int root = -1) {
    vis.assign(n, false);
    is_cut.assign(n, false);
    is_bridge.assign(m, false);
    stk.clear();
    comps.clear();
    tour = 0;

    if (0 <= root && root < n)
      dfs(root, -1);

    for (int i = 0; i < n; i++)
      if (!vis[i])
        dfs(i, -1);
  }
};

/* Note: instead of a block-cut tree this is technically a block-vertex tree, which ends up being much easier to use.
 * Usage:
 *  bc_tree bct(bc);
 *  bct.build();
 */
struct bc_tree {
  bicomps &bc;

  int n, BC, T;
  vector<vector<int>> adj;
  vector<int> parent;
  vector<int> depth;

  // Warning: make sure to call build as well.
  bc_tree(bicomps &_bc) : bc(_bc) {}

  void dfs(int node, int par) {
    parent[node] = par;
    depth[node] = par < 0 ? 0 : depth[par] + 1;

    for (int neigh: adj[node])
      if (neigh != par)
        dfs(neigh, node);
  }

  void build() {
    n = bc.n;
    BC = int(bc.comps.size());
    T = n + BC;
    adj.assign(T, {});

    auto add_edge = [&](int a, int b) {
      assert((a < n) ^ (b < n));
      adj[a].push_back(b);
      adj[b].push_back(a);
    };

    for (int i = 0; i < BC; i++)
      for (int x : bc.comps[i])
        add_edge(x, n + i);

    parent.assign(T, -1);
    depth.resize(T);

    for (int root = 0; root < T; root++)
      if (parent[root] < 0)
        dfs(root, -1);
  }

  bool same_bicomp(int a, int b) {
    if (depth[a] > depth[b])
      swap(a, b);

    // Two different nodes are in the same biconnected comp iff their distance = 2 in the block-cut tree.
    return a == b || (depth[b] == depth[a] + 2 && parent[parent[b]] == a) || (parent[a] >= 0 && parent[a] == parent[b]);
  }
};

