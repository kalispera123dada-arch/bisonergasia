-- Test 15: Valid IN / NOT IN with type-compatible literals
CREATE TABLE products (
    prod_id int,
    category varchar(50),
    price float
);

-- varchar column with string literals (OK)
SELECT *
FROM products
WHERE category IN ('Electronics', 'Clothing', 'Food');

-- int column with integer literals (OK)
SELECT *
FROM products
WHERE prod_id NOT IN (10, 20, 30);
