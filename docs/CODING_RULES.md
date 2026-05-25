# C++ Coding Style & OOP Rules

A definitive reference based on the **C++ Core Guidelines** (Stroustrup & Sutter) and **Google C++ Style Guide**.

---

## 1. Naming Conventions

| Element | Convention | Example |
|---------|-----------|---------|
| Class / Struct / Enum / Type alias | `PascalCase` | `BankAccount`, `PlayerStats` |
| Function / Method | `camelCase` | `calculateTotal()`, `getName()` |
| Local variable / Parameter | `camelCase` | `itemCount`, `userName` |
| Member variable | `camelCase_` (trailing underscore) | `health_`, `name_` |
| Constant / `constexpr` | `kPascalCase` | `kMaxSize`, `kPi` |
| Macro / `#define` | `SCREAMING_SNAKE_CASE` | `BUFFER_SIZE` |
| Namespace | `lowercase` | `engine`, `utils` |
| File | `snake_case.cpp` / `.h` | `player_controller.cpp` |
| Boolean | prefix `is`, `has`, `can`, `should` | `isActive`, `hasPermission` |
| Template parameter | `PascalCase` | `template <typename T>` |
| Enum value (`enum class`) | `PascalCase` | `Color::Red` |

```cpp
class BankAccount {
private:
    double balance_;
    std::string ownerName_;

public:
    static constexpr double kMinimumDeposit = 10.0;

    bool isActive() const;
    void deposit(double amount);
};
```

---

## 2. Formatting Rules

- **Indentation**: 4 spaces. Never tabs.
- **Braces**: K&R / 1TBS style (opening brace on the same line).
- **Line length**: max 100 characters.
- **Pointer style**: `int* ptr;` (asterisk attached to type).
- **Spaces**: around binary operators, after commas, after keywords.

```cpp
if (condition) {
    doSomething();
} else {
    doOther();
}

int sum = a + b;
foo(a, b, c);
int* ptr = nullptr;
```

---

## 3. OOP Principles

### 3.1 Encapsulation
All data members are `private`. Expose behavior through `public` methods.
```cpp
class Account {
private:
    double balance_;

public:
    void deposit(double amount);
    double getBalance() const;
};
```

### 3.2 Inheritance
Use `public` inheritance for *is-a* relationships. Base classes used polymorphically **must** have a `virtual` destructor.
```cpp
class Animal {
public:
    virtual ~Animal() = default;
    virtual void speak() const = 0;
};

class Dog : public Animal {
public:
    void speak() const override;
};
```

### 3.3 Polymorphism
Always mark overrides with `override`. Mark non-extensible classes/methods `final`.
```cpp
class Circle : public Shape {
public:
    double area() const override;
};
```

### 3.4 Abstraction
Use pure virtual functions to declare interfaces. Prefix interface classes with `I`.
```cpp
class IDrawable {
public:
    virtual void draw() const = 0;
    virtual ~IDrawable() = default;
};
```

---

## 4. Class Design Rules

### Rule of Zero / Three / Five
- **Rule of Zero** (preferred): use standard containers and smart pointers so you never write special members.
- **Rule of Five**: if you write one of these, write all five.

```cpp
class Resource {
public:
    Resource();
    ~Resource();
    Resource(const Resource& other);
    Resource& operator=(const Resource& other);
    Resource(Resource&& other) noexcept;
    Resource& operator=(Resource&& other) noexcept;
};
```

### Member Declaration Order
1. `public:` types and constants
2. `public:` constructors / destructor
3. `public:` methods
4. `protected:` members
5. `private:` methods
6. `private:` data members

### Mandatory Rules
- Use `const` on every method that doesn't modify state.
- Use `explicit` on single-argument constructors.
- Use `= default` and `= delete` instead of empty or hidden special members.
- Initialize members in the **member initializer list**, never in the constructor body.

```cpp
class Player {
public:
    explicit Player(const std::string& name)
        : name_(name), health_(100) {}

    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    std::string getName() const { return name_; }

private:
    std::string name_;
    int health_;
};
```

---

## 5. Memory and Resource Management

- **Never use raw `new` / `delete`.** Use `std::make_unique` and `std::make_shared`.
- `std::unique_ptr` — default choice for ownership.
- `std::shared_ptr` — only when ownership is genuinely shared.
- `std::weak_ptr` — to break `shared_ptr` cycles.
- Use **RAII**: every resource (memory, file, lock, socket) is owned by an object whose destructor releases it.

```cpp
auto player = std::make_unique<Player>("Hero");
```

---

## 6. Best Practices

1. Prefer **composition over inheritance**.
2. Program to **interfaces**, not implementations.
3. One class = one responsibility (SRP).
4. Use `enum class`, never plain `enum`.
5. Pass large objects by `const&`, primitives by value.
6. Return by value — let RVO/move semantics handle it.
7. Use `nullptr`, never `NULL` or `0`.
8. Use `auto` for long/obvious types.
9. Always use `#pragma once` at the top of headers.
10. Mark functions `noexcept` when they cannot throw.

```cpp
#pragma once

#include <string>

class Logger final {
public:
    void log(const std::string& message) noexcept;
};
```

---

## 7. SOLID Principles

| Letter | Principle | Meaning |
|--------|-----------|---------|
| **S** | Single Responsibility | One class, one reason to change. |
| **O** | Open/Closed | Open for extension, closed for modification. |
| **L** | Liskov Substitution | Subclasses must be usable through base pointers/references. |
| **I** | Interface Segregation | Many small interfaces beat one large one. |
| **D** | Dependency Inversion | Depend on abstractions, not concretions. |

---

## 8. Complete Example

```cpp
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace game {

class IWeapon {
public:
    virtual ~IWeapon() = default;
    virtual int getDamage() const = 0;
};

class Sword final : public IWeapon {
public:
    explicit Sword(int damage) : damage_(damage) {}
    int getDamage() const override { return damage_; }

private:
    int damage_;
};

class Player final {
public:
    static constexpr int kMaxHealth = 100;

    explicit Player(const std::string& name)
        : name_(name), health_(kMaxHealth) {}

    void equip(std::unique_ptr<IWeapon> weapon) noexcept {
        weapon_ = std::move(weapon);
    }

    bool isAlive() const noexcept { return health_ > 0; }
    const std::string& getName() const noexcept { return name_; }

private:
    std::string name_;
    int health_;
    std::unique_ptr<IWeapon> weapon_;
};

}  // namespace game
```