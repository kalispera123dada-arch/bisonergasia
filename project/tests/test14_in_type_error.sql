-- Test 14: Semantic error — type mismatch in IN list
-- dept is varchar but compared with integers
CREATE TABLE employees (
    emp_id int,
    dept varchar(50),
    salary float
);

SELECT *
FROM employees
WHERE dept IN (1, 2, 3);
