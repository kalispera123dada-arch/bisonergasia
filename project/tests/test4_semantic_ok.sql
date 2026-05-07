-- Test 4: Full semantic checks (Q2) — all should pass

CREATE TABLE orders (
    order_id int,
    customer varchar(100),
    amount float,
    status varchar(20)
);

/* SELECT with all clauses — all columns are valid */
SELECT order_id, customer
FROM orders
WHERE amount > 100.0 AND status = 'active'
GROUP BY status
ORDER BY customer
LIMIT 5;

/* Negative literal in WHERE */
SELECT *
FROM orders
WHERE amount > -1.5;
