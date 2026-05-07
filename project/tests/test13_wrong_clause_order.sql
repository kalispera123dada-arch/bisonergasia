-- Test 13: Syntax error — wrong clause order (ORDER BY before WHERE)
CREATE TABLE orders (
    order_id int,
    amount float,
    status varchar(20)
);

SELECT order_id
FROM orders
ORDER BY order_id
WHERE amount > 100.0;
