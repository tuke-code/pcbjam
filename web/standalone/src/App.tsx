import { Route, Routes } from "react-router-dom";
import { HomePage } from "@/pages/HomePage";
import { ProjectView } from "@/pages/ProjectView";
import { ToolPage } from "@/pages/ToolPage";

export default function App() {
  return (
    <Routes>
      <Route path="/" element={<HomePage />} />
      <Route path="/p/:project" element={<ProjectView />} />
      <Route path="/p/:project/:tool/*" element={<ToolPage />} />
    </Routes>
  );
}
