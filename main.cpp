#include <algorithm>
#include <cassert>
#include <iostream>
#include <ostream>
#include <set>
#include <utility>
#include <vector>

enum PieceID { NONE, A, B, C, D, E, F };
const char *PieceNames[] = {".", "A", "B", "C", "D", "E", "F"};

// A point is a (x, y, z) coordinate
struct Point {
  int x;
  int y;
  int z;
  bool operator<(const Point &rhs) const {
    return x < rhs.x || (x == rhs.x && y < rhs.y) ||
           (x == rhs.x && y == rhs.y && z < rhs.z);
  }
  bool operator==(const Point &rhs) const {
    return x == rhs.x && y == rhs.y && z == rhs.z;
  }
};
struct Size {
  int x;
  int y;
  int z;
};

// A piece is a set of points
class Piece {
public:
  Piece(PieceID id, std::initializer_list<Point> points)
      : id_{id}, points_(points) {
    normalize();
  }

  // Rotate the piece 90 degrees clockwise around the z-axis
  Piece &rotateZ() {
    std::vector<Point> new_points;
    for (const auto &p : points_) {
      new_points.push_back({p.y, -p.x, p.z});
    }
    points_ = new_points;
    return this->normalize();
  }

  // Rotate the piece 90 degrees clockwise around the x-axis
  Piece &rotateX() {
    std::vector<Point> new_points;
    for (const auto &p : points_) {
      new_points.push_back({p.x, p.z, -p.y});
    }
    points_ = new_points;
    return this->normalize();
  }

  // Rotate the piece 90 degrees clockwise around the y-axis
  Piece &rotateY() {
    std::vector<Point> new_points;
    for (const auto &p : points_) {
      new_points.push_back({p.z, p.y, -p.x});
    }
    points_ = new_points;
    return this->normalize();
  }

  // Normalize the piece, so that the first point is at (0, 0, 0)
  // Also update the bounding box size
  // And sort the points
  Piece &normalize() {
    int min_x = 0;
    int min_y = 0;
    int min_z = 0;
    for (const auto &p : points_) {
      min_x = std::min(min_x, p.x);
      min_y = std::min(min_y, p.y);
      min_z = std::min(min_z, p.z);
    }
    for (auto &p : points_) {
      p.x -= min_x;
      p.y -= min_y;
      p.z -= min_z;
    }
    // Update the bounding box size
    size_.x = 0;
    size_.y = 0;
    size_.z = 0;
    for (const auto &p : points_) {
      size_.x = std::max(size_.x, p.x + 1);
      size_.y = std::max(size_.y, p.y + 1);
      size_.z = std::max(size_.z, p.z + 1);
    }
    // Sort the points
    std::sort(points_.begin(), points_.end());
    return *this;
  }

  // Check if two pieces are equal
  bool operator==(const Piece &rhs) const { return points_ == rhs.points_; }

  // Print to ostream
  friend std::ostream &operator<<(std::ostream &os, const Piece &piece) {
    os << "ID: " << PieceNames[piece.id_] << ", size: [" << piece.size_.x
       << ", " << piece.size_.y << ", " << piece.size_.z << "], points: ["
       << piece.points_.size();
    for (const auto &p : piece.points_) {
      os << "(" << p.x << ", " << p.y << ", " << p.z << ") ";
    }
    os << "]";
    return os;
  }

  // Compare two pieces
  bool operator<(const Piece &rhs) const { return points_ < rhs.points_; }

  PieceID id_;
  std::vector<Point> points_;
  Size size_; // The size of the bounding box
};

// PieceSet is all possible orientations of a piece
using PieceOrients = std::set<Piece>;

// Generate all 24 rotations of a piece
// (6 different x-axis orientation * 4 different rotations around the x-axis)
PieceOrients allRotations(Piece p) {
  // clang-format off
  PieceOrients result;

  // 1st orientation, toward positive x-axis
  result.insert(p);
  p.rotateX(); result.insert(p);
  p.rotateX(); result.insert(p);
  p.rotateX(); result.insert(p);
  // 2nd orientation, toward positive y-axis
  p.rotateZ(); result.insert(p);
  p.rotateY(); result.insert(p);
  p.rotateY(); result.insert(p);
  p.rotateY(); result.insert(p);
  // 3rd orientation, toward negative x-axis
  p.rotateZ(); result.insert(p);
  p.rotateX(); result.insert(p);
  p.rotateX(); result.insert(p);
  p.rotateX(); result.insert(p);
  // 4th orientation, toward negative y-axis
  p.rotateZ(); result.insert(p);
  p.rotateY(); result.insert(p);
  p.rotateY(); result.insert(p);
  p.rotateY(); result.insert(p);
  // 5th orientation, toward positive z-axis
  p.rotateX(); result.insert(p);
  p.rotateZ(); result.insert(p);
  p.rotateZ(); result.insert(p);
  p.rotateZ(); result.insert(p);
  // 6th orientation, toward negative z-axis
  p.rotateX().rotateX(); result.insert(p);
  p.rotateZ(); result.insert(p);
  p.rotateZ(); result.insert(p);
  p.rotateZ(); result.insert(p);
  // clang-format on

  // The box has height 2, so we can filter out some pieces with height > 2
  PieceOrients filtered;
  for (const auto &p : result) {
    if (p.size_.z <= 2) {
      filtered.insert(p);
    }
  }

  return filtered;
}

struct Position {
  int x;
  int y;
  int z;
};

struct PiecePos {
  const Piece *piece;
  Position pos;
};

// A box a 3D space with integer coordinates, maintain a 3D array of bools
struct Box {
  int x;
  int y;
  int z;
  Box(int x, int y, int z) : x(x), y(y), z(z) {
    data = new PieceID[x * y * z];
    for (int i = 0; i < x * y * z; ++i) {
      data[i] = NONE;
    }
  }
  PieceID *data;
  bool isOccupied(int x, int y, int z) const {
    return data[x + y * this->x + z * this->x * this->y];
  }
  void setOccupied(int x, int y, int z, PieceID id) {
    data[x + y * this->x + z * this->x * this->y] = id;
  }
  void clearOccupied(int x, int y, int z) {
    data[x + y * this->x + z * this->x * this->y] = NONE;
  }

  bool hasDupIDPiece() const {
    bool has[7] = {false};
    for (int i = 0; i < pieces.size(); ++i) {
      if (has[pieces[i].piece->id_]) {
        return true;
      }
      has[pieces[i].piece->id_] = true;
    }
    return false;
  }

  bool tryPushPieceTo(const Piece &piece, const Position &pos) {
    for (const auto &p : piece.points_) {
      if (isOccupied(pos.x + p.x, pos.y + p.y, pos.z + p.z)) {
        return false;
      }
    }
    for (const auto &p : piece.points_) {
      setOccupied(pos.x + p.x, pos.y + p.y, pos.z + p.z, piece.id_);
    }
    pieces.push_back({&piece, pos});
    return true;
  }

  std::pair<bool, Position> tryPushOriendtedPiece(const Piece &piece) {
    for (int x = 0; x < this->x - piece.size_.x + 1; ++x) {
      for (int y = 0; y < this->y - piece.size_.y + 1; ++y) {
        for (int z = 0; z < this->z - piece.size_.z + 1; ++z) {
          if (tryPushPieceTo(piece, {x, y, z})) {
            return {true, {x, y, z}};
          }
        }
      }
    }
    return {false, {0, 0, 0}};
  }

  std::pair<bool, Position> tryPushPiece(const PieceOrients &pieceOrients) {
    for (const auto &p : pieceOrients) {
      auto [success, pos] = tryPushOriendtedPiece(p);
      if (success) {
        return {true, pos};
      }
    }
    return {false, {0, 0, 0}};
  }

  void popPiece() {
    const auto &piece = pieces.back();
    for (const auto &p : piece.piece->points_) {
      assert(
          isOccupied(piece.pos.x + p.x, piece.pos.y + p.y, piece.pos.z + p.z));
      clearOccupied(piece.pos.x + p.x, piece.pos.y + p.y, piece.pos.z + p.z);
    }
    pieces.pop_back();
  }

  void printVisualize(std::ostream &os) {
    for (int x = 0; x < this->x; ++x) {
      for (int z = 0; z < this->z; ++z) {
        for (int y = 0; y < this->y; ++y) {
          os << PieceNames[data[x + y * this->x + z * this->x * this->y]];
        }
        os << "  ";
      }
      os << std::endl;
    }
    os << std::endl;
  }
  std::vector<PiecePos> pieces;
};

void searchSolution(const std::vector<PieceOrients> &pieceOrients,
                    int currentPieceOrientsIdx, Box &box,
                    std::vector<Box> &solutions) {
  if (currentPieceOrientsIdx == pieceOrients.size()) {
    // Found a solution
    std::cout << "Found a solution" << std::endl;
    box.printVisualize(std::cout);
    for (const auto &p : box.pieces) {
      std::cout << "  " << *p.piece << " at (" << p.pos.x << ", " << p.pos.y
                << ", " << p.pos.z << ")" << std::endl;
    }
    solutions.push_back(box);
    return;
  }

  // For one piece, try all orientations
  for (const auto &p : pieceOrients[currentPieceOrientsIdx]) {
    // Try to push the piece into the box
    for (int x = 0; x < box.x - p.size_.x + 1; ++x) {
      for (int y = 0; y < box.y - p.size_.y + 1; ++y) {
        for (int z = 0; z < box.z - p.size_.z + 1; ++z) {
          bool success = box.tryPushPieceTo(p, {x, y, z});
          if (success) {
            // Found a position to push the piece
            searchSolution(pieceOrients, currentPieceOrientsIdx + 1, box,
                           solutions);
            // Pop the piece
            box.popPiece();
          }
        }
      }
    }
  }
}

int main() {
  Piece p0{A,
           {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {0, 1, 1}}};
  Piece p1{B, {{0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {2, 1, 0}, {2, 1, 1}}};
  Piece p2{C, {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {1, 1, 1}}};
  Piece p3{D, {{0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {0, 0, 1}}};
  Piece p4{E,
           {{0, 0, 0},
            {1, 0, 0},
            {2, 0, 0},
            {0, 1, 0},
            {1, 1, 0},
            {2, 1, 0},
            {2, 0, 1}}};
  Piece p5{F,
           {{0, 0, 0}, {2, 0, 0}, {0, 1, 0}, {1, 1, 0}, {2, 1, 0}, {2, 0, 1}}};

  std::vector<Piece> pieces{p4, p5, p0, p1, p2, p3};
  std::vector<PieceOrients> pieceOrients;
  for (const auto &p : pieces) {
    pieceOrients.push_back(allRotations(p));
  }

  // Dump all pieces
  for (const auto &s : pieceOrients) {
    std::cout << "Piece set: " << s.size() << std::endl;
    for (const auto &p : s) {
      std::cout << "  " << p << std::endl;
    }
  }

  // Search for solutions
  Box box(4, 4, 2);
  std::vector<Box> solutions;
  searchSolution(pieceOrients, 0, box, solutions);

  std::cout << "Found " << solutions.size() << " solutions" << std::endl;
}
