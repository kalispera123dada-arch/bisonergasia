-- Test 11: Semantic error — duplicate table name
CREATE TABLE products (
    prod_id int,
    price float
);

CREATE TABLE products (
    prod_id int,
    name varchar(50)
);
