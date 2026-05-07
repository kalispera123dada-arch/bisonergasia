-- Test 12: Semantic error — duplicate column in CREATE TABLE
CREATE TABLE employees (
    emp_id int,
    name varchar(100),
    emp_id int
);
