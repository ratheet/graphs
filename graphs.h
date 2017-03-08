#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using std::ostringstream; 
using std::string;
using std::unique_ptr;
using std::vector;
using Value = std::pair<string, int>;

// Forward-declarations.
class DirectedGraph;
class Vertex;

// Concept definitions.
template<typename S>
concept bool Stringable = requires(S s) {
  { s.to_string() } -> string;
};

template<typename T>
concept bool Vertex_ptr = requires(T x) {
  { *x } -> const Vertex&;
};

template<typename G, typename T>
concept bool Graph = 
  Stringable<G> &&
  Vertex_ptr<T> &&
  requires(G&& g, T u, T v) {
  { g.add(u) } -> void;
  { g.add_edge(u, v) } -> void;
  { g.are_adjacent(u, v) } -> bool;
  { g.edge_count() } -> int;
  { g.get_neighbors(u) } -> std::vector<T>;
  { g.remove(u) } -> void;
  { g.vertex_count() } -> int;
  };

// Library functions using concepts.
namespace graph_lib {
  bool adjacent(Graph<Vertex*>&& g, Vertex_ptr& u, Vertex_ptr& v) {
    return g.are_adjacent(u, v);
  }
  
  vector<Vertex*> neighbors(Graph<Vertex*>& g, Vertex_ptr x) {
    return g.get_neighbors(x);
  }
  
  void add(Graph<Vertex*>& g, Vertex_ptr x) {
    g.add(x);
  }

  void remove(Graph<Vertex*>& g, Vertex_ptr x) {
    g.remove(x);
  }

  void add_edge(Graph<Vertex*>& g, Vertex_ptr x, Vertex_ptr y) {
    g.add_edge(x, y);
  }
}

// Class definitions.
class Vertex {
 public:
  Vertex(const Vertex& vertex) : value_(vertex.value_) {}
  Vertex(const Value value) : value_(value) {}
  bool operator==(const Vertex& other) const {
    if (this == &other) {
      return true;
    }
    return this->value_ == other.value_;
  }
  string to_string() const {
    ostringstream oss;
    oss << "(" << this->value_.first << ", " << this->value_.second << ")";
    return oss.str();
  }

 private:
  Value value_;
};

class Edge {
 public:
  Edge() {}
  Edge(unique_ptr<Vertex> source, unique_ptr<Vertex> dest) noexcept
    : source_(std::move(source)), dest_(std::move(dest)) {}
  Edge(Edge&& edge) noexcept
    : source_(std::move(edge.source_)), dest_(std::move(edge.dest_)) {
  }
  Edge(const Edge &edge) noexcept {
    if (edge.source_) {
      source_ = std::make_unique<Vertex>(*(edge.source_.get()));
    }
    if (edge.dest_) {
      dest_ = std::make_unique<Vertex>(*(edge.dest_.get()));
    }
  }
  ~Edge() noexcept {}
  Edge& operator=(Edge &&edge) noexcept {
    if (this != &edge) {
      source_ = std::move(edge.source_);
      dest_ = std::move(edge.dest_);
    }
    return *this;
  }
  Edge& operator=(const Edge&) noexcept = delete;
  bool operator==(const Edge& other) const {
    if (this == &other) {
      return true;
    }
    if (this->source_ && *(this->source_.get()) == *(other.source_.get())) {
      if (this->dest_ && *(this->dest_.get()) == *(other.dest_.get())) {
	return true;
      }
    }
    return false;
  }

  void set_source(const Vertex& v) {
    source_ = std::make_unique<Vertex>(v);
  }

  void set_dest(const Vertex& v) {
    dest_ = std::make_unique<Vertex>(v);
  }
  
  string to_string() const {
    ostringstream oss;
    if (source_) {
      oss << source_->to_string();
    } else {
      oss << "NULL";
    }
    oss << " -> ";
    if (dest_) {
      oss << dest_->to_string();
    } else {
      oss << "NULL";
    }
    oss << "\n";
    return oss.str();
  }

  const unique_ptr<Vertex>& get_source() const {
    return source_;
  }

  const unique_ptr<Vertex>& get_dest() const {
    return dest_;
  }
  
 private:
  unique_ptr<Vertex> source_;
  unique_ptr<Vertex> dest_;
};

class DirectedGraph {
 public:
  string to_string() const {
    string str_value = "Graph (# vertices = " + std::to_string(vertex_count()) + "):\n";
    for (const Edge& e : edges_) {
      str_value += e.to_string() + "\n";
    }
    return str_value;
  }

  void add(const Vertex* v) {
    Edge edge;
    edge.set_source(*v);
    edges_.push_back(edge);
  }

  void add_edge(const Vertex* u, const Vertex* v) {
    Edge edge;
    edge.set_source(*u);
    edge.set_dest(*v);
    edges_.push_back(edge);
  }

  void remove(const Vertex* v) {
    for (Edge& e : edges_) {
      if (e.get_source().get() && *(e.get_source().get()) == *v) {
	// Remove the edge: if the source is gone, the edge to the dest
	// is no longer needed.
	auto it = std::find(edges_.begin(), edges_.end(), e);
	if (it != edges_.end()) {
	  edges_.erase(it);
	}
      }
    }
  }
  
  int vertex_count() const {
    int num_vertices = 0;
    for (const Edge& e : edges_) {
      if (e.get_source().get()) {
	num_vertices++;
      }
      if (e.get_dest().get()) {
	num_vertices++;
      }
    }
    return num_vertices;
  }

  int edge_count() const {
    int num_edges = 0;
    for (const Edge& e : edges_) {
      // An Edge is considered a "true" edge only if it has both a
      // source and a destination.
      if (e.get_source() && e.get_dest()) {
	num_edges++;
      }
    }
    return num_edges;
  }

  bool are_adjacent(Vertex_ptr& u, Vertex_ptr& v) {
    for (const Edge& e : edges_) {
      if (e.get_source() && *(e.get_source().get()) == *u) {
	if (e.get_dest() && *(e.get_dest().get()) == *v) {
	  return true;
	}
      }
    }
    return false;
  }

  vector<Vertex_ptr> get_neighbors(Vertex_ptr& vertex)
      const {
    vector<Vertex*> neighbors;
    for (const Edge& e : edges_) {
      if (e.get_source() && *(e.get_source()) == *vertex) {
	if (e.get_dest()) {
	  neighbors
	    .push_back(std::make_unique<Vertex>(*(e.get_dest().get())).get());
						
	}
      } else if (e.get_dest() && *(e.get_dest()) == *vertex) {
	if (e.get_source()) {
	  neighbors
	    .push_back(std::make_unique<Vertex>(*(e.get_source().get())).get());
	}
      }      
    }
    return neighbors;
  }

 private:
  vector<Edge> edges_;
};
