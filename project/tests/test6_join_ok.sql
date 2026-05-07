-- Test 6: Valid JOIN (Q3)

CREATE TABLE departments (
    dept_id int,
    dept_name varchar(100)
);

CREATE TABLE staff (
    staff_id int,
    full_name varchar(100),
    dept_id int,
    salary float
);

/* Simple JOIN */
SELECT staff.full_name, departments.dept_name
FROM staff
JOIN departments ON staff.dept_id = departments.dept_id
WHERE staff.salary > 50000;

/* Multiple JOINs */
CREATE TABLE projects (
    proj_id int,
    proj_name varchar(100),
    dept_id int
);

SELECT staff.full_name, departments.dept_name, projects.proj_name
FROM staff
JOIN departments ON staff.dept_id = departments.dept_id
JOIN projects ON departments.dept_id = projects.dept_id
ORDER BY staff.full_name
LIMIT 20;
