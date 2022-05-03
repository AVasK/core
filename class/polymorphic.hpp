#pragma once
/**
 * @brief Polymorphic CRTP base to combat the slicing problems
 * 
 */
struct polymorphic {
public:
    // virtual destructor
    virtual ~polymorphic () = default;
protected:
    polymorphic() = default;

    // Enable copy- and move- construction for derived classes
    polymorphic(polymorphic const&) = default;
    polymorphic(polymorphic &&) = default;

    // Delete operator= to prevent slicing
    polymorphic& operator= (polymorphic const&) = delete;
    polymorphic& operator= (polymorphic &&) = delete;
};