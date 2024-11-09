/** @type {import('tailwindcss').Config} */
module.exports = {
  content: ["./views/*.html"],
  theme: {
    extend: {
      fontFamily: {
        comic: ["Comic sans MS", "Comic Sans", "cursive"],
      },
      colors: {
        clifford: "#da373d",
      },
    },
  },
  plugins: [],
};
