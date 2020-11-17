# Abstractions - Serializable

Serializable is a set of (unenforced) restrictions on an object, with the goal of easily translating between Lua objects and [JSON](https://www.json.org/json-en.html).

For an object to meet the Serializable restriction, it must meet the criteria listed below.

## Primitive types only

Can only be one of the following types:

- tables (with many restrictions, see below)
- strings
- numbers
- bool
- nil

## Recursively serializable (for tables)

All table keys and values must be serializable.

## No table reuse (for tables)

Tables must not be a value in more than one table.

## No mixed key types (for tables)

String keys and number keys must not both appear in the same table.

## No sparse arrays (for tables)

Nil values must not appear in tables containing only number keys.
