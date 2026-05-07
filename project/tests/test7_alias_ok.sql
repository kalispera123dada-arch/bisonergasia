-- Test 7: Valid table aliases (Q3)

CREATE TABLE customers (
    cust_id int,
    cust_name varchar(100),
    city varchar(50)
);

CREATE TABLE invoices (
    inv_id int,
    cust_id int,
    total float
);

/* Alias on FROM table */
SELECT c.cust_name, c.city
FROM customers AS c
WHERE c.city = 'Athens';

/* Alias on both tables in JOIN */
SELECT c.cust_name, i.total
FROM customers AS c
JOIN invoices AS i ON c.cust_id = i.cust_id
WHERE i.total > 500.0
ORDER BY c.cust_name
LIMIT 10;
