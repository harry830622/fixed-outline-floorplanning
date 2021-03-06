#include "./contour.hpp"
#include "./floorplan.hpp"

#include <stack>

using namespace std;
using Json = nlohmann::json;

Floorplan::Floorplan(int num_macros, bool is_drawing)
    : is_drawing_(is_drawing),
      width_(0.0),
      height_(0.0),
      wirelength_(0.0),
      b_star_tree_(num_macros),
      macro_id_by_node_id_(num_macros, -1),
      is_macro_rotated_by_id_(num_macros, false),
      macro_bounding_box_by_id_(num_macros,
                                make_pair(Point(0, 0), Point(0, 0))),
      drawing_(Json::object()) {
  for (int i = 0; i < macro_id_by_node_id_.size(); ++i) {
    macro_id_by_node_id_[i] = i;
  }
}

const Json& Floorplan::drawing() const { return drawing_; }

int Floorplan::num_macros() const { return b_star_tree_.num_nodes(); }

double Floorplan::width() const { return width_; }

double Floorplan::height() const { return height_; }

double Floorplan::area() const { return width_ * height_; }

double Floorplan::wirelength() const { return wirelength_; }

const pair<Point, Point>& Floorplan::macro_bounding_box(int macro_id) const {
  return macro_bounding_box_by_id_.at(macro_id);
}

void Floorplan::Perturb(const Database& database) {
  const int num_nodes = b_star_tree_.num_nodes();
  const int num_macros = num_nodes;
  const int op = rand() % 3;
  switch (op) {
    case 0: {
      const int macro_id = [&]() {
        int id = rand() % num_macros;
        while (!database.macro(id).is_rotatable()) {
          id = rand() % num_macros;
        }
        return id;
      }();
      is_macro_rotated_by_id_.at(macro_id) =
          !is_macro_rotated_by_id_.at(macro_id);

      if (is_drawing_) {
        const string& macro_name = database.macro(macro_id).name();
        drawing_["perturbation"] = {{"type", "rotation"},
                                    {"macro", macro_name}};
      }

      break;
    }
    case 1: {
      const int node_a_id = rand() % num_nodes;
      const int node_b_id = [&]() {
        int node_id = rand() % num_nodes;
        while (node_id == node_a_id) {
          node_id = rand() % num_nodes;
        }
        return node_id;
      }();
      int& node_a_macro_id = macro_id_by_node_id_.at(node_a_id);
      int& node_b_macro_id = macro_id_by_node_id_.at(node_b_id);
      swap(node_a_macro_id, node_b_macro_id);

      if (is_drawing_) {
        const string& macro_a_name = database.macro(node_a_macro_id).name();
        const string& macro_b_name = database.macro(node_b_macro_id).name();
        drawing_["perturbation"] = {{"type", "swap"},
                                    {"macroA", macro_a_name},
                                    {"macroB", macro_b_name}};
      }

      break;
    }
    case 2: {
      const int node_a_id = rand() % num_nodes;
      const int node_b_id = [&]() {
        int node_id = rand() % num_nodes;
        while (node_id == node_a_id) {
          node_id = rand() % num_nodes;
        }
        return node_id;
      }();
      auto inserted_positions = make_pair(rand(), rand());
      b_star_tree_.DeleteAndInsert(node_a_id, node_b_id, inserted_positions);

      if (is_drawing_) {
        const int node_a_macro_id = macro_id_by_node_id_.at(node_a_id);
        const int node_b_macro_id = macro_id_by_node_id_.at(node_b_id);
        const string& macro_a_name = database.macro(node_a_macro_id).name();
        const string& macro_b_name = database.macro(node_b_macro_id).name();
        drawing_["perturbation"] = {{"type", "deleteAndInsert"},
                                    {"deletedMacro", macro_a_name},
                                    {"targetMacro", macro_b_name}};
      }

      break;
    }
    default:
      break;
  }
}

void Floorplan::Pack(const Database& database) {
  b_star_tree_.UnvisitAll();

  const int root_id = b_star_tree_.root_id();
  const int root_macro_id = macro_id_by_node_id_.at(root_id);
  const Macro& root_macro = database.macro(root_macro_id);
  double root_macro_width = root_macro.width();
  double root_macro_height = root_macro.height();
  const bool is_root_macro_rotated = is_macro_rotated_by_id_.at(root_macro_id);

  if (is_root_macro_rotated) {
    swap(root_macro_width, root_macro_height);
  }

  Contour contour;
  macro_bounding_box_by_id_.at(root_macro_id) =
      contour.Update(0.0, root_macro_width, root_macro_height);

  if (is_drawing_) {
    drawing_["macros"] = Json::array();
    const auto& root_macro_bounding_box =
        macro_bounding_box_by_id_.at(root_macro_id);
    const Point& root_macro_lower_left = root_macro_bounding_box.first;
    const Point& root_macro_upper_right = root_macro_bounding_box.second;
    const string& root_macro_name = root_macro.name();
    drawing_["macros"].push_back(
        {{"name", root_macro_name},
         {"lowerLeft",
          {{"x", root_macro_lower_left.x()}, {"y", root_macro_lower_left.y()}}},
         {"upperRight",
          {{"x", root_macro_upper_right.x()},
           {"y", root_macro_upper_right.y()}}}});
  }

  stack<int> unvisited_node_ids;
  unvisited_node_ids.push(root_id);
  while (!unvisited_node_ids.empty()) {
    const int current_node_id = unvisited_node_ids.top();
    const int current_macro_id = macro_id_by_node_id_.at(current_node_id);
    const pair<Point, Point> current_macro_bounding_box =
        macro_bounding_box_by_id_.at(current_macro_id);
    const int left_child_id = b_star_tree_.left_child_id(current_node_id);
    const int right_child_id = b_star_tree_.right_child_id(current_node_id);

    if (left_child_id != -1 && !b_star_tree_.is_visited(left_child_id)) {
      unvisited_node_ids.push(left_child_id);

      const int left_child_macro_id = macro_id_by_node_id_.at(left_child_id);
      const Macro& left_child_macro = database.macro(left_child_macro_id);
      double left_child_macro_width = left_child_macro.width();
      double left_child_macro_height = left_child_macro.height();
      const bool is_left_child_macro_rotated =
          is_macro_rotated_by_id_.at(left_child_macro_id);

      if (is_left_child_macro_rotated) {
        swap(left_child_macro_width, left_child_macro_height);
      }

      macro_bounding_box_by_id_.at(left_child_macro_id) =
          contour.Update(current_macro_bounding_box.second.x(),
                         left_child_macro_width, left_child_macro_height);

      if (is_drawing_) {
        const auto& left_child_macro_bounding_box =
            macro_bounding_box_by_id_.at(left_child_macro_id);
        const Point& left_child_macro_lower_left =
            left_child_macro_bounding_box.first;
        const Point& left_child_macro_upper_right =
            left_child_macro_bounding_box.second;
        const string& left_child_macro_name = left_child_macro.name();
        drawing_["macros"].push_back(
            {{"name", left_child_macro_name},
             {"lowerLeft",
              {{"x", left_child_macro_lower_left.x()},
               {"y", left_child_macro_lower_left.y()}}},
             {"upperRight",
              {{"x", left_child_macro_upper_right.x()},
               {"y", left_child_macro_upper_right.y()}}}});
      }
    } else if (right_child_id != -1 &&
               !b_star_tree_.is_visited(right_child_id)) {
      unvisited_node_ids.push(right_child_id);

      const int right_child_macro_id = macro_id_by_node_id_.at(right_child_id);
      const Macro& right_child_macro = database.macro(right_child_macro_id);
      double right_child_macro_width = right_child_macro.width();
      double right_child_macro_height = right_child_macro.height();
      const bool is_right_child_macro_rotated =
          is_macro_rotated_by_id_.at(right_child_macro_id);

      if (is_right_child_macro_rotated) {
        swap(right_child_macro_width, right_child_macro_height);
      }

      macro_bounding_box_by_id_.at(right_child_macro_id) =
          contour.Update(current_macro_bounding_box.first.x(),
                         right_child_macro_width, right_child_macro_height);

      if (is_drawing_) {
        const auto& right_child_macro_bounding_box =
            macro_bounding_box_by_id_.at(right_child_macro_id);
        const Point& right_child_macro_lower_right =
            right_child_macro_bounding_box.first;
        const Point& right_child_macro_upper_right =
            right_child_macro_bounding_box.second;
        const string& right_child_macro_name = right_child_macro.name();
        drawing_["macros"].push_back(
            {{"name", right_child_macro_name},
             {"lowerLeft",
              {{"x", right_child_macro_lower_right.x()},
               {"y", right_child_macro_lower_right.y()}}},
             {"upperRight",
              {{"x", right_child_macro_upper_right.x()},
               {"y", right_child_macro_upper_right.y()}}}});
      }
    } else {
      unvisited_node_ids.pop();

      b_star_tree_.Visit(current_node_id);
    }
  }

  width_ = contour.max_x();
  height_ = contour.max_y();

  wirelength_ = 0.0;
  for (int i = 0; i < database.num_nets(); ++i) {
    wirelength_ += database.net(i).ComputeWirelength(macro_bounding_box_by_id_);
  }

  if (is_drawing_) {
    drawing_["width"] = width();
    drawing_["height"] = height();
    drawing_["area"] = area();
    drawing_["wirelength"] = wirelength();
  }
}
