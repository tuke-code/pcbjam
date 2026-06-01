import { Route, Routes } from "react-router-dom";
import { ProjectsPage } from "@/pages/ProjectsPage";
import { ProjectDetailPage } from "@/pages/ProjectDetailPage";
import { ToolPage } from "@/pages/ToolPage";

export default function App() {
  return (
    <Routes>
      <Route path="/" element={<ProjectsPage />} />
      <Route path="/p/:project" element={<ProjectDetailPage />} />
      <Route path="/p/:project/:tool/*" element={<ToolPage />} />
    </Routes>
  );
}
