#include <algorithm>
#include <cassert>
#include <iostream>
#include <ostream>
#include <set>
#include <utility>
#include <vector>

enum PieceID { NONE, H7, O6, F5, Y5, V5, U5, N5, L5, Z5, O4, T4, Z4, L4 };
const char *PieceNames[] = {".",  "H7", "O6", "F5", "Y5", "V5", "U5",
                            "N5", "L5", "Z5", "O4", "T4", "Z4", "L4"};

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
       << ", " << piece.size_.y << ", " << piece.size_.z << "], points: [ ";
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
using PieceOrientsPtr = std::set<Piece> *;

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

  // The box has height 1, so we can filter out some pieces with height > 1
  PieceOrients filtered;
  for (const auto &p : result) {
    if (p.size_.z <= 1) {
      filtered.insert(p);
    }
  }

  return filtered;
}

struct Position {
  int x;
  int y;
  int z;
  friend std::ostream &operator<<(std::ostream &os, const Position &pos) {
    os << "(" << pos.x << ", " << pos.y << ", " << pos.z << ")";
    return os;
  }
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
    data.resize(x * y * z);
    for (int i = 0; i < x * y * z; ++i) {
      data[i] = NONE;
    }
  }
  std::vector<PieceID> data;
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

  bool isOutOfBound(const Position &pos) const {
    return pos.x < 0 || pos.x >= x || pos.y < 0 || pos.y >= y || pos.z < 0 ||
           pos.z >= z;
  }

  bool tryPushPieceTo(const Piece &piece, const Position &pos) {
    for (const auto &p : piece.points_) {
      if (isOutOfBound({pos.x + p.x, pos.y + p.y, pos.z + p.z})) {
        return false;
      }
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

  void printVisualize(std::ostream &os) const {
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

  // TODO(yoco): an optional initial position
  Position findFirstEmptyCell() {
    for (int x = 0; x < this->x; ++x) {
      for (int y = 0; y < this->y; ++y) {
        for (int z = 0; z < this->z; ++z) {
          if (!isOccupied(x, y, z)) {
            return {x, y, z};
          }
        }
      }
    }
    return {-1, -1, -1};
  }

  std::vector<PiecePos> pieces;

  // Output to ostream
  friend std::ostream &operator<<(std::ostream &os, const Box &box) {
    os << "Box: [" << box.x << ", " << box.y << ", " << box.z
       << "], pieces: " << box.pieces.size() << std::endl;
    for (const auto &p : box.pieces) {
      std::cout << "  Pos (" << p.pos.x << ", " << p.pos.y << ", " << p.pos.z
                << ") " << *p.piece << std::endl;
    }
    box.printVisualize(os);
    return os;
  }
};

void searchOnePieceAllPosOrient(const std::vector<PieceOrients> &pieceOrients,
                                int currentPieceOrientsIdx, Box &box,
                                std::vector<Box> &solutions) {
  // Found a solution
  if (currentPieceOrientsIdx == pieceOrients.size()) {
    solutions.push_back(box);
    return;
  }

  // For one piece, try all orientations
  for (const auto &p : pieceOrients[currentPieceOrientsIdx]) {
    // For all positions in the box
    for (int x = 0; x < box.x - p.size_.x + 1; ++x) {
      for (int y = 0; y < box.y - p.size_.y + 1; ++y) {
        for (int z = 0; z < box.z - p.size_.z + 1; ++z) {
          // Try to push the piece into the box
          bool success = box.tryPushPieceTo(p, {x, y, z});
          if (success) {
            // search for the next piece
            searchOnePieceAllPosOrient(pieceOrients, currentPieceOrientsIdx + 1,
                                       box, solutions);
            // Pop the piece
            box.popPiece();
          }
        }
      }
    }
  }
}

// TODO(yoco): an optional initial position
void searchNextCellPiece(int level,
                         const std::vector<PieceOrientsPtr> &pieceOrientPtrs,
                         Box &box, std::vector<Box> &solutions) {
  auto printIndent = [level]() {
    for (int i = 0; i < level; ++i) {
      std::cout << "  ";
    }
  };

  // Found a solution
  if (pieceOrientPtrs.empty()) {
    solutions.push_back(box);
    return;
  }

  // Find next empty cell in the box
  // TODO(yoco): an optional initial position
  Position emptyCell = box.findFirstEmptyCell();
  // printIndent();
  // std::cout << "Empty cell pos: " << emptyCell << std::endl;

  // For each piece, try all orientations, and push to the empty cell
  // For each piece
  for (int i = 0; i < pieceOrientPtrs.size(); ++i) {
    // For each orientation
    for (const auto &p : *pieceOrientPtrs[i]) {
      // Calculate offset: the first point of the piece
      // should be at the empty cell
      Position posToTry = {emptyCell.x - p.points_[0].x,
                           emptyCell.y - p.points_[0].y,
                           emptyCell.z - p.points_[0].z};
      // Try to push the piece into the box
      // printIndent();
      // std::cout << "  Trying pos" << posToTry << ", " << p << std::endl;
      bool success = box.tryPushPieceTo(p, posToTry);
      if (success) {
        // Remove the piece from the piece set
        std::vector<PieceOrientsPtr> newPieceOrients = pieceOrientPtrs;
        newPieceOrients.erase(newPieceOrients.begin() + i);
        // search for the next piece
        searchNextCellPiece(level + 1, newPieceOrients, box, solutions);
        // Pop the piece
        box.popPiece();
      }
    }
  }
}

Point operator""_p(const char *str, std::size_t len) {
  assert(len == 3);
  return {str[0] - '0', str[1] - '0', str[2] - '0'};
}

// clang-format off
int main() {
  // clang-format off
  std::vector<Piece> pieces{
      Piece(H7, {"000"_p, "020"_p, "100"_p, "110"_p, "120"_p, "200"_p, "220"_p}),
      Piece(O6, {"000"_p, "010"_p, "020"_p, "100"_p, "110"_p, "120"_p}),
      Piece(F5, {"000"_p, "010"_p, "110"_p, "120"_p, "210"_p}),
      Piece(Y5, {"000"_p, "010"_p, "020"_p, "030"_p, "110"_p}),
      Piece(V5, {"000"_p, "010"_p, "020"_p, "100"_p, "200"_p}),
      Piece(U5, {"000"_p, "020"_p, "100"_p, "110"_p, "120"_p}),
      Piece(N5, {"000"_p, "010"_p, "020"_p, "120"_p, "130"_p}),
      Piece(L5, {"000"_p, "010"_p, "020"_p, "030"_p, "100"_p}),
      Piece(Z5, {"000"_p, "010"_p, "110"_p, "210"_p, "220"_p}),
      Piece(O4, {"000"_p, "010"_p, "100"_p, "110"_p}),
      Piece(T4, {"000"_p, "010"_p, "020"_p, "110"_p}),
      Piece(Z4, {"000"_p, "010"_p, "110"_p, "120"_p}),
      Piece(L4, {"000"_p, "010"_p, "020"_p, "100"_p}),
  };
  // clang-format on

  std::vector<PieceOrients> pieceOrients;
  for (const auto &p : pieces) {
    if (p.id_ == F5) {
      pieceOrients.push_back(std::set<Piece>{p});
    } else {
      pieceOrients.push_back(allRotations(p));
    }
  }

  std::vector<PieceOrientsPtr> pieceOrientPtrs;
  std::transform(pieceOrients.begin(), pieceOrients.end(),
                 std::back_inserter(pieceOrientPtrs),
                 [](PieceOrients &s) { return &s; });

  // Dump all pieces
  for (const auto &s : pieceOrients) {
    std::cout << "Piece set: " << s.size() << std::endl;
    for (const auto &p : s) {
      std::cout << "  " << p << std::endl;
    }
  }

  // Search for solutions
  Box box(8, 8, 1);
  std::vector<Box> solutions;
  /* searchOnePieceAllPosOrient(pieceOrients, 0, box, solutions); */
  searchNextCellPiece(0, pieceOrientPtrs, box, solutions);
  std::cout << "Found " << solutions.size() << " solutions" << std::endl;
  std::cout << solutions[0];
}
