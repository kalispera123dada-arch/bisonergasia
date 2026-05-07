-- Test 8: Semantic error — column does not exist in table

CREATE TABLE inventory (
    item_id int,
    item_name varchar(100),
    quantity int
);

SELECT item_id, nonexistent_column
FROM inventory
WHERE quantity > 0;
