-- Test 9: Semantic error — type mismatch in WHERE
-- Comparing VARCHAR column with an integer literal

CREATE TABLE users (
    user_id int,
    username varchar(50),
    score float
);

SELECT *
FROM users
WHERE username = 42;
