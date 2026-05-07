-- Test 10: Comments inside queries

/* This is a block comment
   that spans multiple lines */

CREATE TABLE /* inline comment */ products (
    -- this is a column comment
    prod_id int,
    prod_name varchar(255), /* inline block comment */
    price float
);

SELECT prod_id, /* select comment */ prod_name
FROM products
WHERE price > 10.0 -- end-of-line comment
  AND price <= 999.99;
